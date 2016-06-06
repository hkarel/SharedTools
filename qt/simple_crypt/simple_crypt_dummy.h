 
#pragma once 
#ifndef SIMPLECRYPT_H
#define SIMPLECRYPT_H
	
class SimpleCrypt
{
public:
	explicit SimpleCrypt(quint64 key) {}
	QString encryptToString(const QString& plaintext) {return plaintext;}
	QString decryptToString(const QString& cyphertext) {return cyphertext;}

};	
 
#endif // SIMPLECRYPT_H
