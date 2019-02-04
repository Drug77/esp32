FROM node

RUN apt-get update \
    && apt-get install --no-install-recommends --no-install-suggests -y bash \
        git \
    && rm -rf /var/lib/apt/lists/* \
    && yarn global add @vue/cli \
    && yarn global add serve

EXPOSE 8080
WORKDIR /esp32

ENTRYPOINT ["tail", "-f", "/dev/null"]
