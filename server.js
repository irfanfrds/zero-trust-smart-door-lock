const https = require('https');
const fs = require('fs');
const express = require('express');
const db = require('./db'); 
const TelegramBot = require('node-telegram-bot-api').default || require('node-telegram-bot-api');
const basicAuth = require('express-basic-auth');

const app = express();
app.use(express.json());

const pendingSwipes = {};
let isSystemAlarmActive = false;

const telegramToken = ' YOUR_TELEGRAM_BOT_TOKEN';
const bot = new TelegramBot(telegramToken, {polling: true});

console.log("[+] Telegram Bot initialized and polling for actions...");

bot.on('polling_error', (error) => {
    console.error(`[!] Telegram Network Error (Ignored): ${error.code}. Retrying...`);
});

bot.on('callback_query', (callbackQuery) => {
    const message = callbackQuery.message;
    const data = callbackQuery.data; 

    if (data === 'alarm_dismiss') {
        isSystemAlarmActive = false;
        bot.editMessageText(`✅ *Security Alarm Deactivated*\n\nThe physical alarm at the Main Door has been silenced by the administrator.`, {
            chat_id: message.chat.id,
            message_id: message.message_id,
            parse_mode: 'Markdown'
        });
        console.log("[+] System alarm deactivated via Telegram.");
        return bot.answerCallbackQuery(callbackQuery.id, { text: "Alarm Silenced!" });
    }

    const action = data.split('_')[0];
    const rfid_uid = data.split('_')[1];

    if (pendingSwipes[rfid_uid] === 'pending') {
        pendingSwipes[rfid_uid] = (action === 'approve') ? 'approved' : 'denied';
        
        const statusIcon = (action === 'approve') ? '✅' : '❌';
        const statusText = (action === 'approve') ? 'Access Granted' : 'Access Denied';

        bot.editMessageText(`🚨 *Login Request Processed*\nCard UID: \`${rfid_uid}\`\n\nStatus: ${statusIcon} *${statusText}*`, {
            chat_id: message.chat.id,
            message_id: message.message_id,
            parse_mode: 'Markdown'
        });
        
        console.log(`[*] Telegram 2FA completed for card ${rfid_uid}. Result: ${action}`);
        bot.answerCallbackQuery(callbackQuery.id, { text: `System ${statusText}` });
    } else {
        bot.answerCallbackQuery(callbackQuery.id, { text: "This request has expired or was already handled.", show_alert: true });
    }
});

// =========================================================================
// 1. HARDWARE API ROUTES (Bypass Basic Auth so ESP32 can reach them)
// =========================================================================

app.post('/api/swipe', (req, res) => {
    const { rfid_uid } = req.body;
    console.log(`\n[-> INCOMING SWIPE] Received Card UID: ${rfid_uid}`);

    if (!rfid_uid) return res.status(400).json({ error: "Missing Card UID" });

    const verifyQuery = `
        SELECT d.token_value, u.name, up.pairing_status, up.user_id, up.rfid_id 
        FROM User_Pairing up
        JOIN RFID_Cards r ON up.rfid_id = r.rfid_id
        JOIN Digital_Tokens d ON up.token_id = d.token_id
        JOIN Users u ON up.user_id = u.user_id
        WHERE r.card_uid = ?
    `;

    db.query(verifyQuery, [rfid_uid], (err, results) => {
        if (err) {
            console.error("[-] DB Error during swipe verification:", err.message);
            return res.status(500).json({ error: "Database error" });
        }

        if (results.length > 0) {
            const userStatus = results[0].pairing_status.toLowerCase();
            const userName = results[0].name;
            const userId = results[0].user_id;
            const rfidId = results[0].rfid_id;

            if (userStatus === 'suspended') {
                console.log(`[-] LOST MODE TRIGGERED: Denied swipe for ${userName} (Card: ${rfid_uid})`);
                
                db.query(
                    'INSERT INTO Access_Logs (user_id, rfid_id, result, timestamp) VALUES (?, ?, ?, NOW())', 
                    [userId, rfidId, 'Access Denied - Credentials Suspended'],
                    (insertErr) => {
                        if (insertErr) console.error("[-] DB Insert Error (Suspended Card):", insertErr.message);
                    }
                );
                
                return res.status(403).json({ error: "Credentials Suspended" });
            }

            const telegramChatId = results[0].token_value;
            pendingSwipes[rfid_uid] = 'pending';
            console.log(`[!] Card ${rfid_uid} verified. Pushing Telegram 2FA to ${userName}...`);
            
            const opts = {
                parse_mode: 'Markdown',
                reply_markup: {
                    inline_keyboard: [
                        [
                            { text: '✅ Approve Request', callback_data: `approve_${rfid_uid}` },
                            { text: '❌ Deny Request', callback_data: `deny_${rfid_uid}` }
                        ]
                    ]
                }
            };

            bot.sendMessage(telegramChatId, `🚨 *SECURITY ALERT: Access Requested*\n\n*User:* ${userName}\n*Card UID:* \`${rfid_uid}\`\n*Location:* Main Door\n\nDo you authorize this access?`, opts)
                .catch(err => console.error("Telegram API Error:", err.message));

            res.json({ status: "pending", message: "Awaiting Mobile Approval" });
            
        } else {
            console.log(`[-] Unauthorized swipe attempt (Unregistered Card): ${rfid_uid}`);
            
            db.query(
                'INSERT INTO Access_Logs (user_id, rfid_id, result, timestamp) VALUES (NULL, NULL, ?, NOW())', 
                [`Access Denied - Unregistered Card (${rfid_uid})`],
                (insertErr) => {
                    if (insertErr) console.error("[-] DB Insert Error (Unregistered Card):", insertErr.message);
                }
            );
            
            res.status(401).json({ error: "Invalid Card" });
        }
    });
});

app.post('/api/poll', (req, res) => {
    const { rfid_uid } = req.body;
    
    if (!pendingSwipes[rfid_uid]) return res.json({ status: "none" }); 

    const currentStatus = pendingSwipes[rfid_uid];

    if (currentStatus === 'approved' || currentStatus === 'denied') {
        const logQuery = `
            SELECT up.user_id, up.rfid_id
            FROM User_Pairing up
            JOIN RFID_Cards r ON up.rfid_id = r.rfid_id
            WHERE r.card_uid = ?
        `;
        
        const isApproved = (currentStatus === 'approved');
        const logResultText = isApproved ? 'Access Granted' : 'Access Denied - Mobile Rejected';
        const responseStatus = isApproved ? 'Access Granted' : 'Access Denied';
        const httpCode = isApproved ? 200 : 401;

        db.query(logQuery, [rfid_uid], (err, results) => {
            if (err) {
                console.error("[-] SQL Error in Poll Route:", err.message);
                delete pendingSwipes[rfid_uid];
                return res.status(httpCode).json({ status: responseStatus }); 
            }

            const userId = (results && results.length > 0) ? results[0].user_id : null;
            const rfidId = (results && results.length > 0) ? results[0].rfid_id : null;

            db.query(
                'INSERT INTO Access_Logs (user_id, rfid_id, result, timestamp) VALUES (?, ?, ?, NOW())', 
                [userId, rfidId, logResultText], 
                (insertErr) => {
                    if (insertErr) console.error("[-] DB Insert Error in Poll Route:", insertErr.message);
                    
                    delete pendingSwipes[rfid_uid]; 
                    return res.status(httpCode).json({ status: responseStatus });
                }
            );
        });
        
    } else {
        res.json({ status: "pending" });
    }
});

app.get('/api/alarm-status', (req, res) => {
    res.json({ alarm: isSystemAlarmActive });
});

app.post('/api/alert', (req, res) => {
    const { message } = req.body;
    console.log(`[ALARM] ${message}`);
    
    isSystemAlarmActive = true; 
    const adminChatId = '654215462'; 
    
    db.query(
        'INSERT INTO Access_Logs (user_id, rfid_id, result, timestamp) VALUES (NULL, NULL, ?, NOW())',
        [`LOCKDOWN ALARM: ${message}`],
        (insertErr) => {}
    );

    const alertText = `⚠️ *SECURITY LOCKDOWN* ⚠️\n\n` +
                      `${message}\n\n` +
                      `*Location:* Main Door\n` +
                      `*Status:* 🚨 SIREN ACTIVE\n\n` +
                      `🚓 *Tap to Call Police:*\n` +
                      `• MERS Emergency: 999\n` +
                      `• IPD Kajang: 0389114222`;

    const opts = {
        parse_mode: 'Markdown',
        reply_markup: {
            inline_keyboard: [[ { text: '🔕 Turn Off Alarm', callback_data: 'alarm_dismiss' } ]]
        }
    };

    bot.sendMessage(adminChatId, alertText, opts)
        .catch(err => console.error("Telegram Alert Error:", err.message));
        
    res.json({ status: "Alert Sent" });
});

// =========================================================================
// 2. ADMIN AUTHENTICATION & PROTECTED WEB DASHBOARD ROUTES BELOW
// =========================================================================

const adminAuth = basicAuth({
    users: { 'admin' : 'YOUR_PASSWORD'};
    challenge: true,
    unauthorizedResponse: '🚨 UNAUTHORIZED ACCESS: Zero Trust Lockdown Initiated'
});

app.post('/api/register', adminAuth, (req, res) => {
    const { name, card_uid, token_value } = req.body;
    if (!name || !card_uid || !token_value) return res.status(400).json({ error: "All fields required" });

    db.query('INSERT INTO Users (name, email, role, status) VALUES (?, "N/A", "User", "active")', [name], (err, userResult) => {
        if (err) return res.status(500).json({ error: "Failed to create user." });
        const userId = userResult.insertId;

        db.query('INSERT INTO RFID_Cards (card_uid, card_status) VALUES (?, "active")', [card_uid], (err, rfidResult) => {
            if (err) return res.status(500).json({ error: "Failed to register RFID card." });
            const rfidId = rfidResult.insertId;

            db.query('INSERT INTO Digital_Tokens (token_value, device_id, token_status) VALUES (?, "Telegram_App", "active")', [token_value], (err, tokenResult) => {
                if (err) return res.status(500).json({ error: "Failed to register digital token." });
                const tokenId = tokenResult.insertId;

                db.query('INSERT INTO User_Pairing (user_id, rfid_id, token_id, pairing_status) VALUES (?, ?, ?, "active")', [userId, rfidId, tokenId], (err) => {
                    if (err) return res.status(500).json({ error: "Failed to pair credentials." });
                    res.json({ message: `Successfully registered and secured: ${name}` });
                });
            });
        });
    });
});

app.get('/api/logs', adminAuth, (req, res) => {
    const query = `
        SELECT l.log_id, COALESCE(u.name, 'Unknown Device/User') AS name, 
               COALESCE(r.card_uid, 'N/A') AS card_uid, l.result, l.timestamp 
        FROM Access_Logs l
        LEFT JOIN Users u ON l.user_id = u.user_id
        LEFT JOIN RFID_Cards r ON l.rfid_id = r.rfid_id
        ORDER BY l.log_id DESC
    `;
    db.query(query, (err, results) => {
        if (err) return res.status(500).json({ error: "Database error" });
        res.json(results);
    });
});

app.get('/api/users', adminAuth, (req, res) => {
    const query = `
        SELECT u.name, r.card_uid, up.pairing_status 
        FROM User_Pairing up
        JOIN Users u ON up.user_id = u.user_id
        JOIN RFID_Cards r ON up.rfid_id = r.rfid_id
    `;
    db.query(query, (err, results) => {
        if (err) return res.status(500).json({ error: "Database error" });
        res.json(results);
    });
});

app.post('/api/toggle-lost-mode', adminAuth, (req, res) => {
    const { card_uid, new_status } = req.body; 
    if (!card_uid || !new_status) return res.status(400).json({ error: "Missing card UID or status" });

    const updateQuery = `
        UPDATE User_Pairing up
        JOIN RFID_Cards r ON up.rfid_id = r.rfid_id
        SET up.pairing_status = ? 
        WHERE r.card_uid = ?
    `;
    
    db.query(updateQuery, [new_status.toLowerCase(), card_uid], (err, result) => {
        if (err) return res.status(500).json({ error: "Database error" });
        res.json({ message: `Status updated to ${new_status}` });
    });
});

app.use('/', adminAuth, express.static('public'));

const options = {
    key: fs.readFileSync('./certs/server.key'),
    cert: fs.readFileSync('./certs/server.crt'),
    ca: fs.readFileSync('./certs/ca.crt'), 
    requestCert: true,                     
    rejectUnauthorized: false             
};

const PORT = process.env.PORT || 3000;
https.createServer(options, app).listen(PORT, '0.0.0.0', () => {
    console.log(`SECURE mTLS Server running on port ${PORT}`);
    console.log(`Primary Wi-Fi: YOUR_WIFI_IP_ADDRESS:${PORT}');
    console.log(`Hotspot Wi-Fi: YOUR_WIFI_IP_ADDRESS:${PORT}');
});