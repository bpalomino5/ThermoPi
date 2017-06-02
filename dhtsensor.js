var sensor = require('node-dht-sensor');

sensor.read(22, 4, function(err, temperature, humidity) {
    if (!err) {
        temperature = temperature * 9/5.0 + 32
	console.log('temp: ' + temperature.toFixed(1) + '°F, ' +
            'humidity: ' + humidity.toFixed(1) + '%'
        );
    }
});
