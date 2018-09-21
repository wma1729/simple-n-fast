#!/bin/sh

set -eu

INTERMEDIATE_KEY_PASSWD=${INTERMEDIATE_KEY_PASSWD:-"5ubP@55phr@53"}

usage() {
	echo signcert.sh
	echo "    [-i <csr-file>]"
	echo "    [-c <common-name>]"
	echo "    [-t server|client]"
	echo
	echo Signs the certificate with intermediate CA.
	echo
	exit 1
}

infile=
cn=
type=

while [ $# -gt 0 ]
do
	case $1 in
		-c) cn=$2; shift ;;
		-i) infile=$2; shift ;;
		-t)
			if [ $2 = "server" ]
			then
				type="server_cert"
			elif [ $2 = "client" ]
			then
				type="client_cert"
			else
				echo "invalid certificate type"
				usage
			fi
			shift ;;
		*) usage ;;
	esac
	shift
done

ignore=${cn:? "common name not specified"}
ignore=${infile:? "CSR file not specified"}
ignore=${type:? "certificate type not specified"}

openssl ca -batch -config intermediate-ca.conf -passin pass:${INTERMEDIATE_KEY_PASSWD} -notext -days 375 -extensions ${type} -in ${infile} -out intermediate/newcerts/${cn}.cert.pem
if [ $? -eq 0 ]
then
	mkdir ${cn}
	cp intermediate/newcerts/${cn}.cert.pem ${cn}
	cp intermediate/certs/chain.cert.pem ${cn}
	tar cf ${cn}.tar ${cn}
	rm -rf ${cn}
	echo "Extract ${cn}.tar to fetch certificate and CA certificate chain."
fi
