const router = require('express').Router();
const keyController = require('../controller/key.controller');

router.post('/create', keyController.create);
router.get('/', keyController.getAll);
router.get('/paging', keyController.getPaging);
router.put('/:id', keyController.update);
router.delete('/:id', keyController.delete);

module.exports = router;