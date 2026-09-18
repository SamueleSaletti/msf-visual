const express = require('express');
const http = require('http');
const { Server } = require("socket.io");
const { spawn } = require('child_process');

const app = express();
const server = http.createServer(app);
const io = new Server(server);

app.use(express.static('public')); // Qui metterai il file index.html

let cProcess = null;

io.on('connection', (socket) => {
    socket.on('start_process', (data) => {
        // Qui in realtà dovresti prima gestire il caricamento dei due file da parte dell'utente
        // e passarli come argomenti
        cProcess = spawn('./msf.out', ['minimo.gr', 'minimo.mm']);

        cProcess.stdout.on('data', (data) => {
            const lines = data.toString().split('\n');
            for(let line of lines) {
                if(line.trim() !== '') {
                    try {
                        const json = JSON.parse(line);
                        socket.emit('update', json); // Manda il json al frontend
                    } catch (e) {
                        console.log("Ignorato log non-JSON: ", line);
                    }
                }
            }
        });
    });

    socket.on('resume_threads', () => {
        if(cProcess) {
            cProcess.stdin.write("START\n"); // Sblocca il fgets nel C
        }
    });
});

server.listen(3000, () => {
    console.log('Server in ascolto su http://localhost:3000');
});