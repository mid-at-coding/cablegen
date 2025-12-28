const koffi = require('koffi')

const cablegen = koffi.load('../../cablegen.so')

const cablegen_test = cablegen.func('bool test(void)')

function test_cablegen(){
	return cablegen_test();
}

module.exports = {
	test_cablegen: function(){
		return test_cablegen();
	}
}
