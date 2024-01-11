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
    isActive: {
        type: Boolean,
        default: false
    },
    device1:{
        type: String,
        default: ""
    },
    device2:{
        type: String,
        default: ""
    },
    device3:{
        type: String,
        default: ""
    },
    device4:{
        type: Object,
        default: ""
    },
    device5:{
        type: Object,
        default: ""
    },
    device6:{
        type: Object,
        default: ""
    },
});

module.exports = mongoose.model('Key', keySchema);