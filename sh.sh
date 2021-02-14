docker build --tag soulmate-dev .
docker run -v $PWD:/project -p 8081:8081 -it --rm soulmate-dev sh
