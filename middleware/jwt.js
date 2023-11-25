// create jwt middleware
const jwt = require('jsonwebtoken');
const jwtMiddleware = (req, res, next) => {
    const token = req.headers['authorization'];
    if (!token) {
        return res.status(401).json({ message: "token not found" })
    }
    jwt.verify(token, 'secret', (err, data) => {
        if (err) {
            return res.status(401).json({ message: "token invalid" })
        }
        req.userId = data._id;
        next();
    })
}
module.exports = jwtMiddleware;