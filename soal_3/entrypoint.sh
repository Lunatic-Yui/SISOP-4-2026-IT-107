#!/bin/bash

if ! command -v rsyslogd &> /dev/null; then
    apt-get update && apt-get install -y rsyslog samba-vfs-modules
fi

mkdir -p /var/log/samba
touch /var/log/samba/raw.log
touch /var/log/samba/libraryit.log
chmod 777 /var/log/samba/raw.log /var/log/samba/libraryit.log

echo "local7.* /var/log/samba/raw.log" > /etc/rsyslog.d/samba-audit.conf
service rsyslog restart

mkdir -p /libraryit/ebooks /libraryit/papers /libraryit/sourcecode /libraryit/docs

userdel $(id -un 1000) 2>/dev/null
groupdel $(getent group 50 | cut -d: -f1) 2>/dev/null
groupdel $(getent group 100 | cut -d: -f1) 2>/dev/null

groupadd -g 50 staff
groupadd -g 100 readonly

useradd -M -u 1000 -s /sbin/nologin -c "" member
(echo "member123"; echo "member123") | smbpasswd -a -s member

useradd -M -u 1001 -s /sbin/nologin -c "" contributor
(echo "contrib456"; echo "contrib456") | smbpasswd -a -s contributor

useradd -M -u 1002 -s /sbin/nologin -c "" librarian
(echo "lib789"; echo "lib789") | smbpasswd -a -s librarian

usermod -aG staff librarian
usermod -aG staff contributor
usermod -aG readonly member

chown -R root:staff /libraryit
chmod 775 /libraryit/ebooks /libraryit/papers /libraryit/docs
chmod 750 /libraryit/sourcecode

smbd -F -d 2