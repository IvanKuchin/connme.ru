FROM ubuntu:xenial as libwebsocket

RUN apt update && apt install --no-install-recommends --no-install-suggests -y \
			build-essential \
			ca-certificates \
			cmake \
			git \
			libssl-dev \
			pkg-config \

		&& git clone -b v2.2.0 --depth 1 https://github.com/warmcat/libwebsockets.git \
		&& cd /libwebsockets \
		&& mkdir build \
		&& cd build \
		&& cmake -DLWS_WITHOUT_DAEMONIZE:BOOL=FALSE -DLWS_WITH_STATIC=1 -DLWS_WITH_SHARED=1 -DCMAKE_INSTALL_PREFIX:PATH=/usr/local/share/libwebsockets/ .. \
		&& make -j4 install \

		&& apt remove --purge -y \
			build-essential \
			cmake \
			git \
		&& apt autoremove -y \
		&& apt clean




FROM ubuntu:xenial

LABEL maintainer="ikuchin"
LABEL org.opencontainers.image.source=https://github.com/ivankuchin/connme.ru

# RUN echo mysql-server mysql-server/root_password password root | debconf-set-selections
# RUN echo mysql-server mysql-server/root_password_again password root | debconf-set-selections

# following two commands suppose to be reused from cache
RUN apt update &&  apt install --no-install-recommends --no-install-suggests -y git ca-certificates

COPY --from=libwebsocket /usr/local/share/libwebsockets/ /usr/local/share/libwebsockets/

RUN git clone --depth 1 https://github.com/IvanKuchin/connme.ru.git /backend
WORKDIR /backend

RUN apt-get install --no-install-recommends --no-install-suggests -y software-properties-common \
        && add-apt-repository -y ppa:jonathonf/ffmpeg-4 \
        && apt update \
        && apt-get install --no-install-recommends --no-install-suggests -y \ 
			build-essential \
			cmake \
	        ffmpeg \
	        graphicsmagick-libmagick-dev-compat \
	        libavcodec-dev \
	        libavformat-dev \
	        libavutil-dev \
	        libcurl4-openssl-dev \
	        libgraphicsmagick++-dev \
	        libhpdf-dev \
	        libmaxminddb-dev \
	        libmysqlclient-dev \
			libssl-dev \
	        libwebp-dev \
			pkg-config \
	        # mysql-server

		&& mkdir build \
		&& cd build \
		&& cmake .. \
		&& make -j4 \
		# && make install

		&& apt remove --purge -y \
			build-essential \
			cmake \
			git \
		&& apt autoremove -y \
		&& apt clean

# COPY my_script.sh /root/
# COPY sql /root/
# COPY pre.sql /root/
# RUN  chmod +x /root/my_script.sh

# forward request and error logs to docker log collector
#RUN ln -sf /dev/stdout /var/log/nginx/access.log \
#       && ln -sf /dev/stderr /var/log/nginx/error.log

STOPSIGNAL SIGTERM

# CMD ["/root/my_script.sh"]

