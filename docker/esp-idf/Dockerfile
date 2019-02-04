
FROM ubuntu

RUN apt-get update
RUN apt-get install -y git wget make libncurses-dev flex bison gperf python python-pip python-pkg-resources python-serial \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir /esp
WORKDIR /esp

RUN wget https://dl.espressif.com/dl/xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz
RUN tar -xzf xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz && rm -f xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz
ENV PATH=$PATH:/esp/xtensa-esp32-elf/bin

RUN git clone --recursive https://github.com/espressif/esp-idf.git
RUN python -m pip install --user -r /esp/esp-idf/requirements.txt
ENV IDF_PATH=/esp/esp-idf

#RUN git clone https://github.com/espressif/esp-idf-template.git esp32
WORKDIR /esp32

ENTRYPOINT ["tail", "-f", "/dev/null"]
