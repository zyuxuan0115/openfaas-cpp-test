#!/usr/bin/bash
AUTH=23bc46b1-71f6-4ed5-8c54-816aa4f8c502:123zO3xZCLrMN6v2BKK1dXYFpXlPkccOFqm12CdAsMgRU4VrNZ9lyGVCGuMDGIwP
APIHOST=130.127.133.209:32001
FUNC=register-user-with-id
#wsk action delete $FUNC
#sleep 5
#wsk action create $FUNC --docker zyuxuan0115/sn-register-user
curl -u $AUTH "http://$APIHOST/api/v1/namespaces/_/actions/$FUNC?blocking=true&result=true" \
-X POST -H "Content-Type: application/json" \
-d '{"first_name":"Tom","last_name":"Wenisch","username":"twenisch","password":"umichandgoogle","user_id":11028}'

curl -u $AUTH "http://$APIHOST/api/v1/namespaces/_/actions/$FUNC?blocking=true&result=true" \
-X POST -H "Content-Type: application/json" \
-d '{"first_name":"Todd","last_name":"Austin","username":"todda","password":"uwandupenn","user_id":11029}'
