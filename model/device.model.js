const mongoose = require('mongoose');

const deviceSchema = mongoose.Schema({
    device1: Number,
    device2: Number,
    device3: Number,
    device4: Number,
    device5: Number,
    device6: Number,
    isAlive: Boolean,
});
module.exports = mongoose.model('Device', deviceSchema);