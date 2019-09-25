FROM alpine:3.10.2

ARG JPP_VERSION=2.0.0-rc3
RUN apk add --update --no-cache --virtual=build-deps \
    boost-dev g++ make cmake libexecinfo-dev linux-headers protobuf-dev \
    && wget -q https://github.com/ku-nlp/jumanpp/releases/download/v${JPP_VERSION}/jumanpp-${JPP_VERSION}.tar.xz \
    && tar xf jumanpp-${JPP_VERSION}.tar.xz \
    && cd jumanpp-${JPP_VERSION}/ \
    && mkdir bld \
    && cd bld \
    && cmake .. -DCMAKE_BUILD_TYPE=Release \
    && make -j $(nproc) \
    && make install \
    && rm -rf /var/cache/* \
    && apk del build-deps \
    && apk add --update --no-cache boost \
    && cd / \
    && rm jumanpp-${JPP_VERSION}.tar.xz \
    && rm -rf jumanpp-${JPP_VERSION}

CMD ["jumanpp"]
