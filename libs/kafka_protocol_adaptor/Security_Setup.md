# 2-way TLS authentication
[2-way TLS authentication](https://docs.oracle.com/cd/E19424-01/820-4811/aakhe/index.html) enables client authentication and data encryption based on TLS/SSL certificates. This document provides instructions for setting up SSL security for kafka communication, as supported in DeepStream. Instructions are provided based on the [Confluent's kafka offering](https://www.confluent.io/); but high level procedure applies to other distributions of kafka as well. Note that TLS has supplanted SSL, through the terms are used interchangeably in literature.


## Install kafka broker (skip if you already have a broker)
Install kafka broker using confluent kafka distribution based on [Quickstart local](https://docs.confluent.io/current/quickstart/ce-quickstart.html#ce-quickstart) recipe.

Create topic and verify that deepstream kafka client (eg: test app for kafka) is able to connect to your broker setup without security.

## Setup broker security (skip if signed broker certificate is already available)
These steps (a) setup a CA, (b) create certificate for the broker (c) signs the certificate with the CA. Follow the steps as required for your setup depending on whether you have a CA and/or signed broker certificate already setup.

### Create CA 
Note that this section creates your own CA for broker certificate signing. In a real world deployment, it is likely that an established third party CA will be used based on [certificate signing request(CSR)](https://en.wikipedia.org/wiki/Certificate_signing_request).

Example:  
```
openssl req -new -x509 -keyout ca-key -out ca-cert -days 500
```

Note:  
- ca-key is the file where the generated private key that is associated with the certificate is stored (user is prompted for password to protect this)  
- ca-cert is certificate
 
### Create certificate for broker and add to keystore 
Example:
```
keytool -keystore kafka.server1.keystore.jks -alias brokerkey -genkey
```
Note:  
- While entering information requested upon running this command, ensure that CN matches the fully qualified domain name (FQDN) of the server.  
- This command creates both a key and keystore; and adds key to keystore  
- Kafka.server1.keystore.jks is the keystore file  
- brokerkey is the alias name of the key that is generated and added to the keystore  

### Export the certificate from the keystore:
```
keytool -keystore kafka.server1.keystore.jks -alias brokerkey -certreq -file cert-file-server1
```
 
### Sign broker certificate using CA
```
openssl x509 -req -CA ca-cert -CAkey ca-key -in cert-file-server1 -out cert-signed-server1 -days 500 -CAcreateserial
```
Note:  
Use password for ca key provided when generating the CA

### Import CA cert into keystore & truststore
```
keytool -keystore kafka.server1.keystore.jks  -alias CARoot -import -file ca-cert
keytool -keystore kafka.server1.truststore.jks  -alias CARoot -import -file ca-cert
 ```

### Import signed broker cert into keystore
keytool -keystore kafka.server1.keystore.jks  -alias brokerkey -import -file cert-signed-server1

## Setup deepstream client security
### Create client CA (skip if desired CA already exists)  
Note that this section creates a CA for the deepstream client. In a real world deployment, it is likely that an established third party CA will be used. In such a case a certificate signing process (CSR) will be used.

Example:
```
openssl req -new -x509 -keyout ca-client-key -out ca-client-cert -days 500
```

### Create certificate for deepstream client and add to client keystore 
```
keytool -keystore kafka.client1.keystore.jks -alias dskey -genkey
```

### Export the certificate from the client keystore:
```
keytool -keystore kafka.client1.keystore.jks -alias dskey -certreq -file cert-file-client1
 ```
 
### Sign DS application certificate using client CA (either the CA you created in earlier step, or one you already had)
```
openssl x509 -req -CA ca-client-cert -CAkey ca-client-key -in cert-file-client1 -out cert-signed-client1 -days 500 -CAcreateserial
```
 
Import client CA cert into client keystore
```
keytool -keystore kafka.client1.keystore.jks  -alias CARoot -import -file ca-client-cert
```

Import signed broker cert into keystore
```
keytool -keystore kafka.client1.keystore.jks  -alias dskey -import -file cert-signed-client1
```

### Convert jks keys to pkcs format on client
This step is required since deepstream (librdkafka) only supports [pkcs#12](https://en.wikipedia.org/wiki/PKCS_12) key format


Step 1 : Create p12 format of keystore  
```
keytool -importkeystore -srckeystore ./kafka.client1.keystore.jks -destkeystore ./kafka.client1.keystore.p12 -deststoretype pkcs12
```

Step 2 : Export the Private Key as a PEM file
```
openssl pkcs12 -in kafka.client1.keystore.p12  -nodes -nocerts -out client1_private_key.pem
```

Step 3 : Exporting the Certificate
```
openssl pkcs12 -in kafka.client1.keystore.p12 -nokeys -out client1_cert.pem
```

### Ensure librdkafka has been built with SSL
Kafka support in DeepStream is built using the librdkafka library. Ensure that librdkafka has been built with SSL support as described in the README.


### Copy client CA to the broker trust store
Copy client CA certificate (ca-client-cert) to the broker.
Add certificate to the broker truststore:
```
keytool -keystore kafka.server1.truststore.jks  -alias CAClientRoot -import -file ca-client-cert
```

## Configure broker settings to use SSL
Configure kafka broker to use SSL based on instructions in the [Configure Brokers](https://docs.confluent.io/current/security/security_tutorial.html#configure-brokers) section of this Confluent Quickstart security tutorial.

The main configuration file for kafka is server.properties located within <install path>/etc/kafka
The tutorial describes changes to be made to this file to enable SSL.

The relevant modified contents of server.properties is shown below. User should modify fields such as paths and password as appropriate while using this as reference.
```
# Enable SSL security protocol  
listeners=SSL://:9093 
security.inter.broker.protocol=SSL
ssl.client.auth=required

# Broker security settings
ssl.truststore.location=/var/ssl/private/kafka.server1.truststore.jks
ssl.truststore.password=test1234
ssl.keystore.location=/var/ssl/private/kafka.server1.keystore.jks
ssl.keystore.password=test1234
ssl.key.password=test1234
 

# ACLs
authorizer.class.name=kafka.security.auth.SimpleAclAuthorizer
super.users=User:kafkabroker
allow.everyone.if.no.acl.found=true
```

Note: 
- While the tutorial the enables secure communication between kafka and zookeeper as well (using SASL), this document does not enable this functionality and doing so is left as an option to the user.  
- For sake of simplicity, the example enables authorization for authenticated users to access kafka broker topics if no relevant ACL is found based on the allow.everyone.if.no.acl.found entry. User should modify this to define ACL rules to suit their needs before deploying their broker. Refer to [Authorization for kafka broker](https://docs.confluent.io/current/kafka/authorization.html) documentation for details.

## DeepStream Application Changes and Configuration
The kafka test application needs to be modified to use as part of the connection operation the correct broker address and port used by the broker for SSL based connections as configured in server.properties file. Kafka support in DeepStream is built around the librdkafka library. The kafka configuration options provided by the kafka message adaptor to the librdkafka library needs to be modified for SSL. The DeepStream documentation describes various mechanisms to provide these config options, but this document addresses these steps based on using a dedicated config file name config.txt. Note that the config file used by the kafka test application is defined at the top of the file as part of the CFG_FILE macro.

A few parameters need to be defined as using the proto-cfg entry within the message-broker section in this config file as shown in the example below.
```
[message-broker]
proto-cfg = "security.protocol=ssl;ssl.ca.location=<path to your ca>/ca-client-cert;ssl.certificate.location=<path to your certificate >/client1_cert.pem;ssl.key.location=<path to your private key>/client1_private_key.pem;ssl.key.password=test1234;debug=broker,security"
```
The various options specified in the config file are described below:
security.protocol.ssl: use SSL as the authentication protocol  
```ssl.ca.location```: path where your client CA certificate is stored  
```ssl.certificate.location```: path where your client certificate is stored  
```ssl.key.location```: path where your protected private key is stored  
```ssl.key.password```: password for your private key provided while extracting it from the p12 file  
```debug```: enable debug logs for selected categories

Additional SSL based options for rdkafka can be provided here such as the cipher suite used (ssl.cipher.suites) and also non-security related options such as debug. Refer to the [librdkafka configuration page](https://github.com/edenhill/librdkafka/blob/master/CONFIGURATION.md) for a complete list of options.

### Run the application 
Run the application as before:
```
./test_kafka_proto_sync
```

If all went well, you should see logs relating to connection being established, SSL handshake and ultimately the messages being sent.

## Viewing messages
You can run a kafka consumer to receive messages being sent. 
Confluent kafka is distributed with a kafka consumer named kafka-console-consumer that you can use for testing based on the command line below:
```
 bin/kafka-console-consumer --bootstrap-server <address>:9093 --topic <your topic> --consumer.config etc/kafka/client-ssl.properties 
```


# References
Kafka security tutorial:[https://docs.confluent.io/current/security/security_tutorial.html](https://docs.confluent.io/current/security/security_tutorial.html)

Authorization using ACLs:[https://docs.confluent.io/current/kafka/authorization.html](https://docs.confluent.io/current/kafka/authorization.html)

Rdkafka Configuration:[https://github.com/edenhill/librdkafka/blob/master/CONFIGURATION.md](https://github.com/edenhill/librdkafka/blob/master/CONFIGURATION.md)


