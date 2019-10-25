#!/bin/bash
set -e

TAG=$(git log -1 --pretty=%h)
LATEST="latest"
ORG="deipworld"
IMAGE_PREFIX="oa-testnet"
NODE_PREFIX="node"
FULL_NODE_PREFIX="full-node"
WALLET_PREFIX="wallet"

echo "Build low-memory node"
export IMAGE_NAME="$ORG/$IMAGE_PREFIX-$NODE_PREFIX:$TAG"
export LATEST_IMAGE_NAME="$ORG/$IMAGE_PREFIX-$NODE_PREFIX:$LATEST"

docker build -t=${IMAGE_NAME} -f docker/Dockerfile.node --build-arg BUILD_TESTNET=ON .
docker tag ${IMAGE_NAME} ${LATEST_IMAGE_NAME}

echo "Build full node"
export IMAGE_NAME="$ORG/$IMAGE_PREFIX-$FULL_NODE_PREFIX:$TAG"
export LATEST_IMAGE_NAME="$ORG/$IMAGE_PREFIX-$FULL_NODE_PREFIX:$LATEST"

docker build -t=${IMAGE_NAME} -f docker/Dockerfile.fullnode --build-arg BUILD_TESTNET=ON .
docker tag ${IMAGE_NAME} ${LATEST_IMAGE_NAME}

echo "Build wallet"
export IMAGE_NAME="$ORG/$IMAGE_PREFIX-$WALLET_PREFIX:$TAG"
export LATEST_IMAGE_NAME="$ORG/$IMAGE_PREFIX-$WALLET_PREFIX:$LATEST"

docker build -t=${IMAGE_NAME} -f docker/Dockerfile.wallet --build-arg BUILD_TESTNET=ON .
docker tag ${IMAGE_NAME} ${LATEST_IMAGE_NAME}

echo "Remove intermediate images"
docker image prune --filter label=stage=intermediate

echo "Push all images"
# docker login --username=${DOCKER_USER} --password=${DOCKER_PASSWORD}
docker push "$ORG/$IMAGE_PREFIX-$NODE_PREFIX:$TAG"
docker push "$ORG/$IMAGE_PREFIX-$NODE_PREFIX:$LATEST"
docker push "$ORG/$IMAGE_PREFIX-$FULL_NODE_PREFIX:$TAG"
docker push "$ORG/$IMAGE_PREFIX-$FULL_NODE_PREFIX:$LATEST"
docker push "$ORG/$IMAGE_PREFIX-$WALLET_PREFIX:$TAG"
docker push "$ORG/$IMAGE_PREFIX-$WALLET_PREFIX:$LATEST"

docker rmi "$ORG/$IMAGE_PREFIX-$NODE_PREFIX:$TAG"
docker rmi "$ORG/$IMAGE_PREFIX-$NODE_PREFIX:$LATEST"
docker rmi "$ORG/$IMAGE_PREFIX-$FULL_NODE_PREFIX:$TAG"
docker rmi "$ORG/$IMAGE_PREFIX-$FULL_NODE_PREFIX:$LATEST"
docker rmi "$ORG/$IMAGE_PREFIX-$WALLET_PREFIX:$TAG"
docker rmi "$ORG/$IMAGE_PREFIX-$WALLET_PREFIX:$LATEST"
