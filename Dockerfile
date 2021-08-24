FROM alpine:latest AS builder
RUN apk add alpine-sdk cmake libexecinfo-dev protobuf-dev curl
COPY . /root
RUN /bin/sh /root/script/make-jpp-alpine-x64.sh

FROM alpine:latest as runner
RUN apk add --no-cache libexecinfo libstdc++
COPY --from=builder /root/bld-docker-dist/usr/local /usr/local
ENTRYPOINT ["/usr/local/bin/jumanpp"]