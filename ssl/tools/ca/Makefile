ROOT_KEY_PASSWD ?= r00tP@55w0rd
INTERMEDIATE_KEY_PASSWD ?= 5ubP@55phr@53
OCSP_KEY_PASSWD ?= 0c5pP@55w0rd
KEYBITS ?= 2048
C ?= US
ST ?= Minnesota
L ?= Mahtomedi
O ?= www.simplenfast.org
OU ?= Department of Perennial Learning

all:	root/private/ca.key.pem \
	root/certs/ca.cert.pem \
	intermediate/private/ca.key.pem \
	intermediate/certs/ca.cert.pem \
	intermediate/certs/chain.cert.pem \
	intermediate/private/ocsp.key.pem \
	intermediate/certs/ocsp.cert.pem

.PHONY: clean sign revoke gencrl

clean:
	/bin/rm -rf root intermediate

root/private/ca.key.pem:
	mkdir -p root/certs
	mkdir -p root/db
	mkdir -p root/private
	chmod 0700 root/private
	openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:$(KEYBITS) -pkeyopt rsa_keygen_pubexp:65537 -aes128 -pass pass:$(ROOT_KEY_PASSWD) -outform pem -out $@

root/certs/ca.cert.pem: root/private/ca.key.pem
	touch root/db/index.txt
	openssl rand -hex 16 > root/db/serial
	echo 1001 > root/db/crlnumber
	openssl req -new -config root-ca.conf -key $^ -keyform pem -passin pass:$(ROOT_KEY_PASSWD) -subj "/C=$(C)/ST=$(ST)/L=$(L)/O=$(O)/OU=$(OU)/CN=RootCA" -out root/ca.csr.pem
	openssl ca -batch -selfsign -config root-ca.conf -passin pass:$(ROOT_KEY_PASSWD) -notext -extensions v3_ca -in root/ca.csr.pem -out $@
	/bin/rm -f root/ca.csr.pem

intermediate/private/ca.key.pem:
	mkdir -p intermediate/certs
	mkdir -p intermediate/crl
	mkdir -p intermediate/db
	mkdir -p intermediate/private
	mkdir -p intermediate/newcerts
	chmod 0700 intermediate/private
	openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:$(KEYBITS) -pkeyopt rsa_keygen_pubexp:65537 -aes128 -pass pass:$(INTERMEDIATE_KEY_PASSWD) -outform pem -out $@

intermediate/certs/ca.cert.pem: intermediate/private/ca.key.pem
	touch intermediate/db/index.txt
	openssl rand -hex 16 > intermediate/db/serial
	echo 1000 > intermediate/db/crlnumber
	openssl req -new -config intermediate-ca.conf -key $^ -keyform pem -passin pass:$(INTERMEDIATE_KEY_PASSWD) -subj "/C=$(C)/ST=$(ST)/L=$(L)/O=$(O)/OU=$(OU)/CN=IntermediateCA" -out intermediate/ca.csr.pem
	openssl ca -batch -config root-ca.conf -passin pass:$(ROOT_KEY_PASSWD) -notext -extensions v3_intermediate_ca -in intermediate/ca.csr.pem -out $@
	/bin/rm -f intermediate/ca.csr.pem

intermediate/certs/chain.cert.pem: intermediate/certs/ca.cert.pem root/certs/ca.cert.pem
	cat $^ > $@

intermediate/private/ocsp.key.pem:
	openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:$(KEYBITS) -pkeyopt rsa_keygen_pubexp:65537 -aes128 -pass pass:$(OCSP_KEY_PASSWD) -outform pem -out $@

intermediate/certs/ocsp.cert.pem: intermediate/private/ocsp.key.pem intermediate/certs/ca.cert.pem
	openssl req -new -config intermediate-ca.conf -key $< -keyform pem -passin pass:$(OCSP_KEY_PASSWD) -subj "/C=$(C)/ST=$(ST)/L=$(L)/O=$(O)/OU=$(OU)/CN=OCSP Responder" -out intermediate/ocsp.csr.pem
	openssl ca -batch -config intermediate-ca.conf -passin pass:$(INTERMEDIATE_KEY_PASSWD) -notext -days 375 -extensions ocsp_ext -in intermediate/ocsp.csr.pem -out $@
	/bin/rm -f intermediate/ocsp.csr.pem

sign:
ifndef CSR
	$(error variable CSR, for Certificate Signing Request, is not set)
endif
ifndef CN
	$(error variable CN, for Common Name, is not set)
endif
ifndef EXT
	$(error variable EXT, for extension to use, is not set)
endif
	openssl ca -batch -config intermediate-ca.conf -passin pass:$(INTERMEDIATE_KEY_PASSWD) -notext -days 375 -extensions $(EXT) -in $(CSR) -out intermediate/newcerts/$(CN).cert.pem
	mkdir $(CN)
	cp intermediate/newcerts/$(CN).cert.pem $(CN)
	cp intermediate/certs/chain.cert.pem $(CN)
	tar cvf $(CN).tar $(CN)
	rm -rf $(CN)
	echo "Extract $(CN).tar to fetch certificate and CA certificate chain."

revoke:
ifndef CERT
	$(error variable CERT, for certificate to revoke, is not set)
endif
	openssl ca -batch -config intermediate-ca.conf -passin pass:$(INTERMEDIATE_KEY_PASSWD) -revoke intermediate/newcerts/$(CERT)

gencrl:
	openssl ca -batch -config intermediate-ca.conf -passin pass:$(INTERMEDIATE_KEY_PASSWD) -gencrl -out intermediate/crl/crl.pem
