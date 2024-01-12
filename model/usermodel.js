const mongoose = require('mongoose');

const userSchema = mongoose.Schema({
    username: String,
    password: String,
    faceID: String,
    phoneNumber: String,
    avatar: String,
    email: String,
});
module.exports = mongoose.model('User', userSchema);