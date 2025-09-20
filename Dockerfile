########################################################################################################################
# Build stage for cligen, clixon and clixon-backend-helper
########################################################################################################################
FROM python:alpine AS clixon_build

# For clixon and cligen
RUN apk update \
    && apk add --no-cache \
        git \
        make \
        build-base \
        gcc \
        flex \
        bison \
        curl-dev \
        nghttp2 \
        net-snmp \
        net-snmp-dev \
        shadow \
        meson \
        swig \
    \
    && adduser -D -H -G www-data www-data \
    && mkdir -p /clixon/build

# Clone and build cligen
RUN git clone https://github.com/clicon/cligen.git /clixon/cligen \
    && cd /clixon/cligen \
    && ./configure --prefix=/usr/local --sysconfdir=/etc \
    && make -j4 \
    && make install \
    && make DESTDIR=/clixon/build install

# Clone and build clixon
RUN git clone https://github.com/clicon/clixon.git /clixon/clixon \
    && cd /clixon/clixon \
    && ./configure --prefix=/usr/local --sysconfdir=/etc --with-cligen=/clixon/build/usr/local --with-restconf=native --enable-nghttp2 --enable-http1 --enable-netsnmp \
    && make -j4 \
    && make install \
    && make install-include \
    && make DESTDIR=/clixon/build install \
    && make DESTDIR=/clixon/build install-include 

# Build clixon backend helper
COPY clixon-backend-plugin /clixon/clixon-backend-helper
WORKDIR /clixon/clixon-backend-helper
RUN cd /clixon/clixon-backend-helper \
    && meson setup build \
    && meson compile -C build \
    && meson install -C build --destdir /clixon/build 


########################################################################################################################
# clixon Docker image
########################################################################################################################
FROM python:alpine

# For clixon and cligen
RUN apk update \
    && apk add --no-cache \
        multirun \
        flex \
        bison \
        openssl \
        nghttp2 \
        net-snmp \
        net-snmp-tools \
        openssh \
        iproute2 \
        nginx \
        libsmi \
    && adduser -D -H -G www-data www-data \
    && adduser -D -H clicon \
    && sed -i 's/^worker_processes.*/worker_processes 1;daemon off;/' /etc/nginx/nginx.conf \
    && sed -i '/^#PermitEmptyPasswords no/c\PermitEmptyPasswords yes' /etc/ssh/sshd_config 

# Configure webssh
RUN pip install webssh

# Copy clixon, cligen and clixon backend helper from build stage
COPY --from=clixon_build /clixon/build/ /

# Some custom configuration for SNMP
RUN echo "master  agentx" > /etc/snmp/snmpd.conf \
    && echo "agentaddress  0.0.0.0" >> /etc/snmp/snmpd.conf \
    && echo "rwcommunity   public  localhost" >> /etc/snmp/snmpd.conf \
    && echo "agentxsocket  unix:/var/run/snmp.sock" >> /etc/snmp/snmpd.conf \
    && echo "agentxperms   777 777" >> /etc/snmp/snmpd.conf \
    && echo "trap2sink     localhost public 162" >> /etc/snmp/snmpd.conf \
    && echo "disableAuthorization yes" >> /etc/snmp/snmptrapd.conf \
    \
    && smidump -f yang /usr/share/snmp/mibs/IF-MIB.txt > /usr/local/share/clixon/IF-MIB.yang

# Configure sshd for CLI (no password required)
RUN mkdir -p /home/cli \
    && adduser -D -s /usr/local/bin/clixon_cli -G clicon -h /home/cli -H cli \
    && passwd -u cli \
    && echo "cli ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

# Configure sshd for netconf (no password required)
RUN echo "Subsystem netconf /usr/local/bin/clixon_netconf" >> /etc/ssh/sshd_config \
    && passwd -u clicon \
    && echo "clicon ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers



# Copy stuff into this container
COPY motd /etc/motd
COPY nginx.conf /etc/nginx/http.d/
COPY index.html /var/www
COPY *.sh /usr/local/bin/
RUN chmod +x /usr/local/bin/*.sh

# Create symlink so you can run clixon without -f arg
RUN ln -s /usr/local/etc/clixon/ietf-ip.xml /etc/clixon.xml 

# Expose https port for restconf
EXPOSE 22/tcp
EXPOSE 80/tcp
EXPOSE 161/udp
EXPOSE 162/udp

# Start daemons
CMD ["/usr/local/bin/start-container.sh"]

