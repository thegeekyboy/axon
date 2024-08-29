## howto keytab

$ ktutil
addent -password -p amirul.islam -k 1 -e aes256-cts-hmac-sha1-96
wkt amirul.islam.kt

$ kinit -kt amirul.islam.kt amirul.islam@BKASH.COM


## rabbit mq init

rabbitmqctl add_user hyperion hyperion
// rabbitmqctl set_user_tags hyperion administrator
// rabbitmqctl set_permissions -p / hyperion ".*" ".*" ".*"

rabbitmqctl add_vhost hyperion
// rabbitmqctl set_permissions -p hyperion guest ".*" ".*" ".*"
rabbitmqctl set_permissions -p hyperion hyperion ".*" ".*" ".*"

rabbitmqadmin -H localhost -u hyperion -p hyperion list vhosts
rabbitmqadmin -H localhost -u hyperion -p hyperion declare exchange --vhost=hyperion name=hyperion type=direct
rabbitmqadmin -H localhost -u hyperion -p hyperion declare queue --vhost=hyperion name=hyperion durable=true
rabbitmqadmin -H localhost -u hyperion -p hyperion declare queue --vhost=hyperion name=completed durable=true
rabbitmqadmin -H localhost -u hyperion -p hyperion --vhost="hyperion" declare binding source="hyperion" destination_type="queue" destination="hyperion" routing_key="hyperion"


### reset
rabbitmqctl delete_vhost hyperion
rabbitmqctl delete_user hyperion