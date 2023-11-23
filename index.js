const ws = require('ws');
const http = require('http');
const url = require('url');
const uuid = require('uuid');


const wss = new ws.Server({ port: 8120 });

const mapDeviceToObj = new Map();
const objToMapDevice = new Map();
const defaultObj = (id) => {
    return {
        id,
        device1: 0,
        device2: 0,
        device3: 0,
        device4: 0,
        device5: 0,
        device6: 0,
    }
}

wss.on('connection', function connection(ws) {
    // get Sec-Websocket-Protocol from client
    const device = ws.protocol;
    if (device === "device") {
        const id = uuid.v4();
        const obj = defaultObj(id);
        mapDeviceToObj.set(id, obj);
        objToMapDevice.set(ws, id);
        ws.send(JSON.stringify(obj));
    } else {
        // get all data in map
        const allDevice = [];
        for (let [key, value] of mapDeviceToObj) {
            allDevice.push(value);
        }
        ws.send(JSON.stringify(allDevice));
    }
    ws.on('message', function incoming(message) {
        // update obj
        try {

            const messageObj = JSON.parse(message);
            // send to this device
            const deviceObj = mapDeviceToObj.get(messageObj.id);
            if (deviceObj) {
                mapDeviceToObj.set(messageObj.id, messageObj);
                // send to all device current device alive
                const allDevice = [];
                for (let [key, value] of mapDeviceToObj) {
                    allDevice.push(value);
                }
                wss.clients.forEach(function each(client) {
                    if (client.readyState === ws.OPEN) {
                        if (!objToMapDevice.get(client)) {

                            client.send(JSON.stringify(allDevice));
                        } else if (objToMapDevice.get(client) === messageObj.id) {
                            client.send(JSON.stringify(messageObj));
                        }
                    }
                });
            } else {
                ws.send("Not found device")
            }
        } catch (e) {
            ws.send("Not found device")
        }
    });

    // disconnect event
    ws.on('close', function close() {
        const id = objToMapDevice.get(ws);
        mapDeviceToObj.delete(id);
        objToMapDevice.delete(ws);
        // send to all device current device alive
        const allDevice = [];
        for (let [key, value] of mapDeviceToObj) {
            allDevice.push(value);
        }
        wss.clients.forEach(function each(client) {
            if (client.readyState === ws.OPEN) {
                if (!objToMapDevice.get(client)) {

                    client.send(JSON.stringify(allDevice));
                }
            }
        });
    });
});
