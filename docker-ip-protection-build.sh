#!/bin/bash
set -e

STAGING_TAG=""
while getopts ":s" opt
do
  case $opt in
    s) STAGING_TAG="staging-";;
  esac
done

TAG=$(git log -1 --pretty=%h)
LATEST="latest"

echo "Building deipworld/ip-protection-platform-full-node $STAGING_TAG image..."
export IMAGE_NAME="deipworld/ip-protection-platform-full-node:${STAGING_TAG}${TAG}"
export LATEST_IMAGE_NAME="deipworld/ip-protection-platform-full-node:${STAGING_TAG}${LATEST}"
docker build -t=${IMAGE_NAME} -f docker/Dockerfile.fullnode --build-arg BUILD_DEIP_TEST_NET=OFF --build-arg LOW_MEMORY_NODE=OFF --build-arg MAKE_BUILD_TYPE=Release .
docker tag ${IMAGE_NAME} ${LATEST_IMAGE_NAME}
docker push ${IMAGE_NAME}
docker push ${LATEST_IMAGE_NAME}
docker rmi ${IMAGE_NAME}
docker rmi ${LATEST_IMAGE_NAME}


# Build local image for prod only
if [ "$STAGING_TAG" == "" ]
then
    echo "Building deipworld/ip-protection-platform-local-node image..."
    export IMAGE_NAME="deipworld/ip-protection-platform-local-node:$TAG"
    export LATEST_IMAGE_NAME="deipworld/ip-protection-platform-local-node:$LATEST"
    docker build -t=${IMAGE_NAME} -f docker/Dockerfile.local --build-arg BUILD_DEIP_TEST_NET=OFF --build-arg LOW_MEMORY_NODE=OFF --build-arg MAKE_BUILD_TYPE=Release .
    docker tag ${IMAGE_NAME} ${LATEST_IMAGE_NAME}
    docker push ${IMAGE_NAME}
    docker push ${LATEST_IMAGE_NAME}
    docker rmi ${IMAGE_NAME}
    docker rmi ${LATEST_IMAGE_NAME}
fi