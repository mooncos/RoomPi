# start from base
FROM alpine:3.11

LABEL maintainer="Marcos Gómez <marcos.gomezb@alumnos.upm.es>"

# Install python/pip
ENV PYTHONUNBUFFERED=1
RUN apk add --no-cache python3 py3-pip
RUN pip3 install --no-cache --upgrade pip setuptools

# We copy just the requirements.txt first to leverage Docker cache
COPY ./requirements.txt /app/requirements.txt

WORKDIR /app

RUN pip3 install -r requirements.txt

COPY . /app

EXPOSE 8082

RUN pwd

CMD [ "python3", "./app.py" ]