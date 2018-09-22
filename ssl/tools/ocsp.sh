#!/bin/sh

cert=
issuer=
chain=
host=
port=

usage() {
	echo
	echo $0
	echo "    -c <certificate>"
	echo "    -i <issuer-certificate>"
	echo "    -t <trusted-certificate-chain>"
	echo "    -h <ocsp-responder-host>"
	echo "    -p <ocsp-responder-port>"
	echo
	echo "Get certificate status from OCSP responder."
	exit 1
}

while [ $# -gt 0 ]
do
	case $1 in
		-c) cert=$2; shift;;
		-i) issuer=$2; shift;;
		-t) chain=$2; shift;;
		-h) host=$2; shift;;
		-p) port=$2; shift;;
		*) usage; shift;;
	esac
	shift
done

ignore=${cert:? "certificate is not specified"}
ignore=${issuer:? "issuer certificate is not specified"}
ignore=${chain:? "trusted certificate chain is not specified"}
ignore=${host:? "OCSP responder host is not specified"}
ignore=${port:? "OCSP responder port is not specified"}

openssl ocsp -no_nonce -no_certs -no_chain -noverify -issuer ${issuer} -VAfile ${chain} -sha256 -cert ${cert} -host ${host} -port ${port}
