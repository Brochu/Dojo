SYSTEM DESIGN - Twitter micro-blog system
=========================================

## 1. Requirments

- DO NOT spend more than 5 minutes listing requirements

### Functional

- My list
    * Post new tweet
    * Re-tweet existing tweet
    * Edit tweet - only before a certain time
    * Like existing tweet

- Hello interivew
    * Create an account + login
    * Create, edit and delete tweets
    * Follow other users
    * View tweets of follows - timeline
    * Like, reply and re-tweet
    * Search for tweets

- Notes to learn
    * don't forget about account & auth
    * combine CRUD requirements
    * touch based w/ interviewer to make sure features are correctly listed

### Non-Functional

- My list
    * Read >>> Writes
    * Low latency media files download (CDN)
    * Low latency for search operations
    * Security, no (write) access to other user's posts
    * 99.99 uptime
    * Backup / fault tolerance

- Hello interivew
    * Scale to 100+ millions users active
    * Handle high volume of tweets, likes, retweets
    * High available (99.999 uptime)
    * Security & privacy of user data
    * Low latency

- Notes to learn
    * remember scaling for amount of users
    * high volume operations

## 2. Design

- Client (web app + mobile)
    * sends requests to the load balancer
- Load balancer (2 topics of discussion in interviews)
    * routing algo (round robin, least connetions, IP hash, ...)
    * layer of operation (layer 4 transport, layer 7 application (can route based on request content))
    * distributes requests across a fleet of servers acting as API gateways
- API Gateway (micro-services arch.)
    * take the incoming requests and will forward it to the correct micro-service
    * will most likely be part of the design in interview
- Services (micro-services arch.)
    * My list
        + Auth/Login
        + Search
        + Timeline
        + Tweets CRUD
    * Hello Interview
        + Tweet CRUD
        + Reply CRUD
        + Search
        + Timeline
        + Profile
        + Auth
    * Notes to learn
        + Sometimes a separate service can help scale ie. replay crud in this case (to handle viral tweets)

- NOTE: Client, Load balancer and API gateway will be present in most of system design interviews

### Tweet CRUD service
- functional reqs
    * Create, edit & delete tweets
    * likes * retweets
    * store metadata, timestamps and the likes
- non-functional
    * very high throughput
    * most frequent operations on twitter
- tweets storage
    * MongoDB, performance efficient, good for a lot of reads
    * id, user_id, username, content, timestamp, likes (list of users), retweets (list of user+timestamps), media, hashtags, mentions, location??, is_reply/retweet_to
    * media will be stored outside of mongo (S3 for object store capabalities) possible CDN blob storage
- Make sure user cannot flood our system with write requests (bot activity)
    * add a rate limiter on the write path of our micro service
- Add a cache on the read path so popular tweets are fast to fetch
- Add CND in order to have data close to users geographic locations

### Reply CRUD service
- replies in a separate document database
    * why? scalability, allows to scale replies indenpendtly
    * can fetch a tweet without replies at first. Faster to see the tweet's content, replies later
- need a rate limiter on the write path again
- don't need an independent read path for replies
    * bundle with the tweet reads
    * indexed by tweet id so reads are fast
- structure: id, tweet_id (reference, index), user_id, username, content, timestamps, likes (list of users), media, hashtags, mentions

### Search service
- could be an interview on it's own, complex system
- we'll go simple for this one
- reverse indexes on tweet's content, username and hashtags using elastic search
    * will let us quickly search tweet's data
- Change Data Capture (CDC) from the tweet DB will send updates to elastic search

### Timeline service
- The algo
- good amount of the interview on this service
- made up of posts from your friends to simplify here, no content recommendation system
- fan-out on read
    * when user request to see timeline
    * get all the accounts the user follows
    * fetch all of their tweets
    * sort by time
    * return the timeline
- slow and expensive, fail non-functional requirements
- instead, we can update the timelines when a user creates a new tweet
    * fan-out on write instead
    * each new tweet created are placed on a message queue, in order to not swamp other parts of the system
    * workers pop tweets off of this message queue to update timelines
    * for each new tweet, we fetch the list of followers on the poster's account
    * each follower will have a timeline cache
    * worker's job is to update this cache for all followers
    * prepend to the cache so more recent tweets are first
    * prioritizes read speed (more writes)
    * issues with large accounts  (lots of followers), need hybrid approach (fan-out on read)
        + for larger account we wait until the user requests the timeline, integrate fan-out on read tweets from big accounts at that time

### Profile service
- create and manage user profiles and followers
- simple SQL database for user data
    * allows for fast query
    * ACID transaction
    * joins for user analytics
- for followers, using a graph DB makes sense because it matches the structure

### Auth service
- separate because it brings better security focused maintenance, integration with 3rd party later is easier

## 3. Security, Monitoring, Testing

### Security
- Authentication * Authorization
    * requests come from legit users
    * user has the right permissions and accesses to the requests
    * handled by the auth service
- Data encryption
    * user HTTPS in transit
    * at rest, native support for encryption for database, flip a switch
- Rate limiting
    * IP rate limiting in the API gateway
    * prevent bots for DDOS
- Input validation
    * prevent XSS, sql injection
    * should be done on both client and server

### Monitoring
- System health checks
    * any component is down, we need to know fast
    * tools like prometheus + grafana
    * real time metrics
- Logging
    * helps debugging, find security breaches
    * tech tech tech
    * Kabana + elastic search
- Alerts (real-time)
    * sudden surge in traffic needs notify
    * pagers duty to quickly alert

### Testing
- Load testing
    * new features need to see their impact on the current system
- Automated testing
    * services need to integrate seamlessly
    * Github actions, unit tests for components
- Backup & Recovery
    * NEED backup
    * test recovery process often
