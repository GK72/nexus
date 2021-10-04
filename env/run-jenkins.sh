#!/usr/bin/env bash

docker run --name jenkins --rm --detach \
  --network jenkins \
  --publish 8080:8080 --publish 50000:50000 \
  --volume /var/run/docker.sock:/var/run/docker.sock \
  --volume jenkins-data:/var/jenkins_home \
  --volume jenkins-docker-certs:/certs/client:ro \
  --volume ${HOME}/repos:${HOME}/repos \
  nxs/ci
