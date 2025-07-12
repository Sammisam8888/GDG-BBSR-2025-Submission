mosquitto_pub -h stagingmqttnossl.txninfra.com -p 1883 -u nosslstage_client2 -P 'client@5645##' -t /comp/9337903728 -m '{"file":"bell-ring.wav","priority":2}'

mosquitto_pub -h stagingmqttnossl.txninfra.com -p 1883 -u nosslstage_client2 -P 'client@5645##' -t /comp/9337903728 -m '{"file":"beep.wav","priority":1}'

mosquitto_pub -h stagingmqttnossl.txninfra.com -p 1883 -u nosslstage_client2 -P 'client@5645##' -t /comp/9337903728 -m '{"file":"explosion.wav","priority":3}'

mosquitto_pub -h stagingmqttnossl.txninfra.com -p 1883 -u nosslstage_client2 -P 'client@5645##' -t /comp/9337903728 -m '{"file":"gunshot.wav","priority":4}'
