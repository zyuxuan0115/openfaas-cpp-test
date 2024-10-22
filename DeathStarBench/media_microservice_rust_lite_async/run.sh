curl localhost:8080/function/register-movie-id -d '{"title":"Barbie","movie_id":"tt1517268"}'
curl localhost:8080/function/write-movie-info -d '{"movie_id":"tt1517268","title":"Barbie","plot_id":113473,"avg_rating":"8.9","num_rating":165314,"casts":[{"cast_id":201,"character":"Kirk Douglas","cast_info_id":12345}],"thumbnail_ids":[],"photo_ids":[],"video_ids":[]}'
curl localhost:8080/function/write-cast-info -d '{"cast_info_id":12345,"name":"Kirk Douglas","gender":true,"intro":"Kirk Douglas was an American actor and filmmaker."}'
curl localhost:8080/function/write-cast-info -d '{"cast_info_id":12346,"name":"Jennifer Lawrence","gender":false,"intro":"Considered one of the most successful actresses of her generation, Lawrence was the highest-paid actress in the world in 2015 and 2016."}'
curl localhost:8080/function/write-plot -d '{"plot_id":113473,"plot":"One evening at a dance party, Barbie is suddenly stricken with worries about mortality. Overnight, she develops bad breath, cellulite, and flat feet, disrupting her routines and impairing the aura of classic perfection experienced by the Barbies. Weird Barbie, a disfigured doll, tells Barbie to find the child playing with her in the real world to cure her afflictions. Barbie follows the advice and travels to the real world, with Ken joining Barbie by stowing away in her convertible."}'
curl localhost:8081/function/register-user -d '{"first_name":"Yuxuan","last_name":"Zhang","username":"zyuxuan","password":"123456"}'
curl localhost:8081/function/register-user-with-id -d '{"user_id":11078,"first_name":"Tom","last_name":"Wenisch","username":"twenisch","password":"12345"}'
curl localhost:8081/function/register-user-with-id -d '{"user_id":11079,"first_name":"Todd","last_name":"Austin","username":"taustin","password":"12345"}'
#curl localhost:8081/function/compose-review-upload-unique-id -d '{"req_id":8037,"review_id":98765}'
#curl localhost:8080/function/text-service -d '{"req_id":8037,"text":"This is a good movie"}'
#curl localhost:8081/function/upload-user-with-username -d '{"username":"zyuxuan","req_id":8037}'
#curl localhost:8080/function/upload-movie-id -d '{"title":"Barbie","rating": 5,"req_id":8037}'
