#!/bin/bash
set -e

TAG=$(git log -1 --pretty=%h)
LATEST="latest"
NODE_PREFIX="node_"
FULL_NODE_PREFIX="fullnode_"
WALLET_PREFIX="wallet_"

echo "Build low-memory node"
export IMAGE_NAME="deipdev/testnet:$NODE_PREFIX$TAG"
export LATEST_IMAGE_NAME="deipdev/testnet:$NODE_PREFIX$LATEST"

docker build -t=${IMAGE_NAME} -f docker/Dockerfile.node --build-arg BUILD_TESTNET=ON .
docker tag ${IMAGE_NAME} ${LATEST_IMAGE_NAME}

echo "Build full node"
export IMAGE_NAME="deipdev/testnet:$FULL_NODE_PREFIX$TAG"
export LATEST_IMAGE_NAME="deipdev/testnet:$FULL_NODE_PREFIX$LATEST"

docker build -t=${IMAGE_NAME} -f docker/Dockerfile.fullnode --build-arg BUILD_TESTNET=ON .
docker tag ${IMAGE_NAME} ${LATEST_IMAGE_NAME}

echo "Build wallet"
export IMAGE_NAME="deipdev/testnet:$WALLET_PREFIX$TAG"
export LATEST_IMAGE_NAME="deipdev/testnet:$WALLET_PREFIX$LATEST"

docker build -t=${IMAGE_NAME} -f docker/Dockerfile.wallet --build-arg BUILD_TESTNET=ON .
docker tag ${IMAGE_NAME} ${LATEST_IMAGE_NAME}

echo "Remove intermediate images"
docker image prune --filter label=stage=intermediate

echo "Push all images"
docker login --username=${DOCKER_USER} --password=${DOCKER_PASSWORD}
docker push "deipdev/testnet"
