#!/bin/bash
set -e

TAG=$(git log -1 --pretty=%h)
LATEST="latest"

echo "Building deipworld/ip-protection-platform-full-node image..."
export IMAGE_NAME="deipworld/ip-protection-platform-full-node:$TAG"
export LATEST_IMAGE_NAME="deipworld/ip-protection-platform-full-node:$LATEST"
docker build -t=${IMAGE_NAME} -f docker/Dockerfile.fullnode --build-arg BUILD_TESTNET=ON .
docker tag ${IMAGE_NAME} ${LATEST_IMAGE_NAME}
docker push ${IMAGE_NAME}
docker push ${LATEST_IMAGE_NAME}