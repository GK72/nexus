var express = require("express");
var request = require("request");
var crypto = require("crypto");
var cors = require("cors");
var querystring = require("querystring");
var cookieParser = require("cookie-parser");

var client_id = process.env.SPOTIFY_CLI_CLIENT_ID;
var client_secret = process.env.SPOTIFY_CLI_CLIENT_SECRET;
var redirect_uri = "http://localhost:8888/callback";

var app = express();

app.get("/login", function(req, res) {
    res.redirect("https://accounts.spotify.com/authorize?" + querystring.stringify({
        response_type: "code",
        client_id: client_id,
        redirect_uri: redirect_uri,
        scope: "playlist-read-private"
    }));
});

app.get("/callback", function(req, res) {
    var code = req.query.code || null;

    var authOptions = {
        url: "https://accounts.spotify.com/api/token",
        form: {
            code: code,
            redirect_uri: redirect_uri,
            grant_type: "authorization_code"
        },
        headers: {
            "content-type": "application/x-www-from-urlencoded",
            Authorization: "Basic " + (new Buffer.from(client_id + ":" + client_secret).toString("base64"))
        },
        json: true
    };

    request.post(authOptions, function(err, resp, body) {
        if (!err && resp.statusCode === 200) {
            var options = {
                url: "https://api.spotify.com/v1/me",
                headers: { "Authorization": "Bearer " + body.access_token },
                json: true
            };

            console.log("Access token: ", body.access_token)
            console.log("Refresh token: ", body.refresh_token)

            request.get(options, function(err, resp, body) {
                console.log(body);
            });
        } else {
            res.redirect("/#" +
                querystring.stringify({
                    error: "invalid_token"
                })
            );
        }
    });
});

app.listen(8888);
