const keyModel = require('../model/key.model');

class KeyController {
    async create(req, res) {
        try {
            // generate key
            const key = await keyModel.create(req.body);
            res.status(201).json(key);
        } catch (err) {
            res.status(500).json(err);
        }
    }

    async getAll(req, res) {
        try {
            const keys = await keyModel.find();
            res.status(200).json(keys);
        } catch (err) {
            res.status(500).json(err);
        }
    }

    async getPaging(res, req) {
        try {
            const { page, limit } = req.query;
            const keys = await keyModel.find()
                .limit(limit * 1)
                .skip((page - 1) * limit)
                .exec();
            const count = await keyModel.countDocuments();
            res.status(200).json({
                keys,
                totalPages: Math.ceil(count / limit),
                currentPage: page
            });
        }catch (err) {
            res.status(500).json(err);
        }
    }
    async update(req, res) {
        try {
            const key = await keyModel.findByIdAndUpdate(req.params.id, req.body, { new: true });
            res.status(200).json(key);
        } catch (err) {
            res.status(500).json(err);
        }
    }
    async getOne(req, res) {
        try {
            const key = await keyModel.findById(req.params.id);
            res.status(200).json(key);
        } catch (err) {
            res.status(500).json(err);
        }
    }
    async delete(req, res) {
        try {
            await keyModel.findByIdAndDelete(req.params.id);
            res.status(204).json();
        } catch (err) {
            res.status(500).json(err);
        }
    }
}
module.exports = new KeyController();