const mongoose = require('mongoose');

const keySchema = mongoose.Schema({
    name: String,
    key: {
        type: String,
        required: true,
        // auto generate key
        default: function () {
            return Math.random().toString(36).substr(2, 5);
        }
    },
});

module.exports = mongoose.model('Key', keySchema);