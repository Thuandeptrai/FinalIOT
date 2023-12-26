const usermodel = require("../model/usermodel");
const fs = require("fs");
const canvas = require("canvas");
const faceApi = require("face-api.js");
const multer = require("multer");
const jwt = require("jsonwebtoken");
const bcryptjs = require("bcryptjs");
const { exec } = require("node:child_process");

class User {
  async createUser(req, res) {
    try {
      console.log(req.file);
      const { username, password } = req.body;
      const usernameDb = await usermodel.findOne({ username: username });
      if (usernameDb) {
        return res.status(400).json({ message: "username already exists" });
      }
      const image1 = req.file.path;
      const faceID = Math.random().toString(36).substring(7);
      const img1 = await canvas.loadImage(image1);
      // label face
      const results = await faceApi.detectAllFaces(img1).withFaceLandmarks().withFaceDescriptors();
      if (!results.length) {
        return res.status(400).json({ message: "no face detected" });
      }
      // check with labeledFaceDescriptors.json to see if this face is already exist
      const labeledFaceDescriptors = JSON.parse(fs.readFileSync("./labeledFaceDescriptors.json"));
      const faceMatcher = new faceApi.FaceMatcher(results);
      let bestMatch = null;
      let savedId = null;
      for (let i = 0; i < labeledFaceDescriptors.length; i++) {
        // compare similarity
        // Error: arr1 and arr2 must have the same length
        const data = faceMatcher.findBestMatch(labeledFaceDescriptors[i].descriptors[0]);
        if (data.distance < 0.5) {
          bestMatch = data;
          savedId = labeledFaceDescriptors[i].label;
          break;
        }
      }
      if(bestMatch !== null) {
        return res.status(400).json({ message: "face already exists" });
      }
      let labeledFaceDescriptors1 = results.map(
        (fd) => new faceApi.LabeledFaceDescriptors(`${faceID}`, [fd.descriptor])
      );
      //fs.writeFileSync('./labeledFaceDescriptors.json', JSON.stringify(labeledFaceDescriptors))
      const file = fs.readFileSync("./labeledFaceDescriptors.json");

      if (file) {
        let labeledFaceDescriptors;
        if (file.length === 0) {
          labeledFaceDescriptors = [];
        } else {
          labeledFaceDescriptors = JSON.parse(file);
        }
        // let labeledFaceDescriptors = [];
        // add id element to object labeledFaceDescriptors1[0]
        labeledFaceDescriptors1[0].id = Math.random().toString(36).substring(7);
        labeledFaceDescriptors.push(labeledFaceDescriptors1[0]);
        // add element id to array labeledFaceDescriptors1
        for (let i = 0; i < labeledFaceDescriptors1.length; i++) {
          labeledFaceDescriptors1[i].id = Math.random().toString(36).substring(7);
        }
        fs.writeFileSync("./labeledFaceDescriptors.json", JSON.stringify(labeledFaceDescriptors));
      } else {
        const labeledFaceDescriptors = [];
        labeledFaceDescriptors.push({ label: "label", descriptors: [singleResult.descriptor] });
        fs.writeFileSync("./labeledFaceDescriptors.json", JSON.stringify(labeledFaceDescriptors));
      }
      // save to mongodbs
      const newUser = new usermodel({
        username: username,
        // hash password
        password: bcryptjs.hashSync(password, 10),
        faceID: faceID
      });
      newUser.save();
      // jwt sign key
      const returnUser = {
        username: username,
        faceID: faceID
      };
      // delete this picture after saves
      exec(`rm ${image1}`);
      return res.status(200).json({ returnUser });
    } catch (e) {
      console.log(e);
      return res.status(500).json({ message: e.message });
    }
  }
}
module.exports = new User();
