#!/bin/bash
set -e

TAG=$(git log -1 --pretty=%h)
LATEST="latest"


export IMAGE_NAME="deipdev/testnet:$TAG"
export LATEST_IMAGE_NAME="deipdev/testnet:$LATEST"

sudo docker build -t=${IMAGE_NAME} -f Dockerfile.testnet .
sudo docker tag ${IMAGE_NAME} ${LATEST_IMAGE_NAME}
sudo doker login --username=${DOCKER_USER} --password=${DOCKER_PASSWORD}
sudo docker push deipdev/testnet
