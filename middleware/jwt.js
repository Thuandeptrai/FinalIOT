// create jwt middleware
const jwt = require('jsonwebtoken');
const jwtMiddleware = (req, res, next) => {
    const token = req.headers['authorization'].split(' ')[1];
    if (!token) {
        return res.status(401).json({ message: "token not found" })
    }
    jwt.verify(token, 'secret', (err, data) => {
        if (err) {
            return res.status(401).json({ message: "token invalid" })
        }
        // find user by id
        req.userId = data._id;
        next();
    })
}
module.exports = jwtMiddleware;