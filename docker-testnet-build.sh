#!/bin/bash
set -e

TAG=$(git log -1 --pretty=%h)
LATEST="latest"


export IMAGE_NAME="deipdev/testnet:$TAG"
export LATEST_IMAGE_NAME="deipdev/testnet:$LATEST"

docker build -t=${IMAGE_NAME} -f Dockerfile.testnet .
docker tag ${IMAGE_NAME} ${LATEST_IMAGE_NAME}
docker login --username=${DOCKER_USER} --password=${DOCKER_PASSWORD}
docker push ${IMAGE_NAME}
docker push ${LATEST_IMAGE_NAME}
