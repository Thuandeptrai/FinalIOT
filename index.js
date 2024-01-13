const ws = require("ws");
const http = require("http");
const user = require("./model/usermodel");
const url = require("url");
const uuid = require("uuid");
const fs = require("fs");
const express = require("express");
const bodyParser = require("body-parser");
const canvas = require("canvas");
const faceApi = require("face-api.js");
const multer = require("multer");
const cors = require("cors");
const jwt = require("jsonwebtoken");
const bcryptjs = require("bcryptjs");
const usermodel = require("./model/usermodel");
const mongoose = require("mongoose");
const { createUser } = require("./controller/user.controller");
const keyRouter = require("./router/key.router");
const key = require("./model/key.model");
const jwtMiddleware = require("./middleware/jwt");
mongoose
  .connect("mongodb://127.0.0.1:27017/face", { useNewUrlParser: true, useUnifiedTopology: true })
  .then(() => {
    console.log("connect mongodb success");
  })
  .catch((err) => {
    console.log(err);
  });

const storage = multer.diskStorage({
  destination: function (req, file, cb) {
    cb(null, "./uploads/");
  },
  filename: function (req, file, cb) {
    const fileName = file.originalname.split(".");
    cb(null, fileName[0] + "-" + Date.now() + "." + fileName[1]);
  }
});

// connect mogodb
const upload = multer({ storage: storage });
// save
// import '@tensorflow/tfjsN-node';
const app = express();
// middle ware to parse json
app.use(express.static("public"));
// allow json
app.use(bodyParser.json());
app.use(
  cors({
    origin: "*"
  })
);
// allow url encoded
app.use(bodyParser.urlencoded({ extended: false }));

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
    isAlive: true
  };
};
// implement facejs api
const { Canvas, Image, ImageData } = canvas;
faceApi.env.monkeyPatch({ Canvas, Image, ImageData });
// load models
const MODELS_URL = "./models";
const faceDetectionOptions = new faceApi.SsdMobilenetv1Options({ minConfidence: 0.5 });
// load models
faceApi.nets.ssdMobilenetv1.loadFromDisk(MODELS_URL);
faceApi.nets.faceLandmark68Net.loadFromDisk(MODELS_URL);
faceApi.nets.faceRecognitionNet.loadFromDisk(MODELS_URL);
// load BBT face similarity via url into endpoint  /url

app.get("/", (req, res) => {});

app.post("/upload", upload.array("images", 2), async (req, res) => {
  try {
    // const get image from request.body.image1
    const image1 = req.files[0].path;
    const image2 = req.files[1].path;
    const img1 = await canvas.loadImage(image1);
    const img2 = await canvas.loadImage(image2);
    // face similarity from image1 and image2
    const results = await faceApi.detectAllFaces(img1).withFaceLandmarks().withFaceDescriptors();
    const faceMatcher = new faceApi.FaceMatcher(results);
    const singleResult = await faceApi.detectSingleFace(img2).withFaceLandmarks().withFaceDescriptor();
    const bestMatch = faceMatcher.findBestMatch(singleResult.descriptor);
    console.log(bestMatch);
    return res.status(200).json({ message: bestMatch.toString() });
  } catch (e) {
    console.log(e);
  }
});
app.post("/signUpWithImage", upload.single("images"), async (req, res) => {
  // signup face and label for face recognition
  createUser(req, res);
});
app.post("/loginWithImage", upload.single("images"), async (req, res) => {
  try {
    // compare similarity upload one image with local image
    const image1 = req.file.path;
    const img1 = await canvas.loadImage(image1);
    let bestMatch = null;
    let savedId = null;
    const results = await faceApi.detectAllFaces(img1).withFaceLandmarks().withFaceDescriptors();
    if (results.length === 0) {
      return res.status(401).json({ message: null });
    }
    const faceMatcher = new faceApi.FaceMatcher(results);

    // load labeledFaceDescriptors from file json to compare
    const file = fs.readFileSync("./labeledFaceDescriptors.json");

    if (file) {
      const parseToObject = JSON.parse(file);
      for (let i = 0; i < parseToObject.length; i++) {
        // compare similarity
        // Error: arr1 and arr2 must have the same length
        const data = faceMatcher.findBestMatch(parseToObject[i].descriptors[0]);
        if (data.distance < 0.5) {
          bestMatch = data;
          savedId = parseToObject[i].label;
          console.log(data);
          break;
        }
      }
    }
    if (bestMatch !== null) {
      console.log(bestMatch);
      const findUser = await usermodel.findOne({ faceID: savedId });
      console.log(findUser);
      const jwt_token = jwt.sign(
        {
          _id: findUser._id
        },
        "secret",
        {
          expiresIn: "1h"
        }
      );
      return res.status(200).json({
        message: {
          username: findUser.username,
          phoneNumber: findUser.phoneNumber || null,
          email: findUser.email || null,
          jwt_token: jwt_token
        }
      });
    }
    return res.status(401).json({ message: "Login Fail" });
  } catch (e) {
    console.log(e);
  }
});
app.post("/loginWithPassword", async (req, res) => {
  const { username, password } = req.body;
  const userName = await usermodel.findOne({ username: username });
  if (!userName) {
    return res.status(400).json({ message: "username not found" });
  }
  const checkPassword = bcryptjs.compareSync(password, userName.password);
  if (!checkPassword) {
    return res.status(400).json({ message: "password invalid" });
  }
  const jwt_token = jwt.sign(
    {
      _id: userName._id
    },
    "secret",
    {
      expiresIn: "1h"
    }
  );
  return res.status(200).json({
    message: {
      username: userName.username,
      phoneNumber: userName.phoneNumber || null,
      email: userName.email || null,
      jwt_token: jwt_token
    }
  });
});
app.put("/editUser", jwtMiddleware, async (req, res) => {
  const { username, phoneNumber,email } = req.body;
  if (!username  || !phoneNumber) {
    return res.status(400).json({ message: "username or password or phoneNumber is null" });
  }
  const user = await usermodel.findOne({ _id: req.userId });
  if (!user) {
    return res.status(400).json({ message: "user not found" });
  }
  user.username = username;
  user.email = email;
  user.phoneNumber = phoneNumber;
  await user.save();
  // remove password
  user.password = undefined;
  return res.status(200).json({ message: "success", user: user });
});
//edit password
app.post("/editPassword", jwtMiddleware, async (req, res) => {
 const {prevPassword, password} = req.body;
  if(!prevPassword || !password){
    return res.status(400).json({message: "prevPassword or password is null"});
  }
  // compare prevPassword with password in database
  const findUser = await usermodel.findOne({_id: req.userId});
  if(!findUser){
    return res.status(400).json({message: "user not found"});
  }
  // const checkPassword = bcryptjs.compareSync(prevPassword, findUser.password);
  // if(!checkPassword){
  //   return res.status(400).json({message: "prevPassword invalid"});
  // }
  // hash password
  findUser.password = bcryptjs.hashSync(password, 10);
  await findUser.save();
  findUser.password = undefined;
  return res.status(200).json({message: "success"});
});
app.use("/key", keyRouter);
app.listen(5010, () => console.log("Server started on port 3000"));

wss.on("connection", async function connection(ws) {
  // get Sec-Websocket-Protocol from client
  const device = ws.protocol;
  // check find key by Sec-Websocket-Protocol
  console.log(ws.protocol);
  const findKey = await key.findOne({ key: device });
  console.log(findKey);
  if (findKey !== null && findKey.isActive === true) {
    ws.send("This key is already in use");
    return ws.terminate();
  }
  if (findKey !== null && findKey.isActive === false) {
    findKey.isActive = true;
    await findKey.save();
  }
  ws.send("Hello, client!"); // send message to client
  if (findKey !== null) {
    ws.isAlive = true;
    const id = ws.protocol;
    const obj = defaultObj(id);
    mapDeviceToObj.set(id, obj);
    objToMapDevice.set(ws, id);
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
    ws.send(JSON.stringify(obj));
  } else {
    // get all data in map
    const allDevice = [];
    for (let [key, value] of mapDeviceToObj) {
      allDevice.push(value);
    }
    ws.send(JSON.stringify(allDevice));
  }
  ws.on("message", function incoming(message) {
    // update obj

    try {
      const messageObj = JSON.parse(message);
      console.log(messageObj);
      // MessageObj type
      // { type: 'message',id device1: 1}
      // {type: ping}
      // send to this device
      if (messageObj.type === "message") {
        let deviceObj = mapDeviceToObj.get(messageObj.id);
        if (deviceObj) {
          // update deviceObj by messageObj i device1: 1 update only device1
          console.log(deviceObj);
          for (let key in messageObj) {
            if (key !== "type" && key !== "id") {
              deviceObj[key] = messageObj[key];
            }
          }
          mapDeviceToObj.set(messageObj.id, deviceObj);
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
                client.send(JSON.stringify(dfeviceObj));
              }
            }
          });
        } else {
          ws.send("Not found device");
        }
      } else if (messageObj.type === "ping") {
        ws.isAlive = true;
        ws.send("pong");
      }
    } catch (e) {
      console.log(e);
      ws.send("Not found device");
    }
  });
  // disconnect event
  ws.on("close", async function close() {
    const id = objToMapDevice.get(ws);
    // update status isAlive
    const update = await key.findOneAndUpdate(
      {
        key: ws.protocol
      },
      { isActive: false }
    );
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
  // check current device alive
  // clear interlval and create new interval
});
setInterval(async function ping() {
  wss.clients.forEach(async function each(ws) {
    console.log(ws.isAlive);
    if (ws.isAlive === false) {
      const id = objToMapDevice.get(ws);
      const updateDevice = await key.findOneAndUpdate(
        {
          key: ws.protocol
        },
        { isActive: false }
      );
      if (!id) {
        return;
      }

      mapDeviceToObj.delete(id);
      objToMapDevice.delete(ws);
      // send to all device current device alive
      const allDevice = [];
      for (let [key, value] of mapDeviceToObj) {
        allDevice.push(value);
      }
      wss.clients.forEach(function each(client) {
        if (client.readyState === ws.OPEN && client !== ws) {
          if (!objToMapDevice.get(client)) {
            client.send(JSON.stringify(allDevice));
          }
        }
      });
      return ws.terminate();
    }
    ws.isAlive = false;
  });
  if (mapDeviceToObj.size === 0) {
    await key.updateMany({}, { isActive: false });
    return;
  }
}, 10000);
