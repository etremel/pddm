/**
 * @file CryptoPPLibrary.cpp
 *
 * @date Oct 31, 2017
 * @author edward
 */

#include "CryptoPPLibrary.h"

#include <mutils-serialization/SerializationSupport.hpp>
#include <cryptopp/files.h>
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>

namespace pddm {

namespace util {

CryptoPP::RSA::PrivateKey CryptoPPLibrary::load_private_key(const std::string& private_key_filename) {
    CryptoPP::RSA::PrivateKey private_key;
    CryptoPP::FileSource priv_key_file(private_key_filename.c_str(), true);
    CryptoPP::ByteQueue temp_queue_1;
    priv_key_file.TransferAllTo(temp_queue_1);
    temp_queue_1.MessageEnd();
    private_key.Load(temp_queue_1);
    return std::move(private_key);
}

CryptoPPLibrary::CryptoPPLibrary(const std::string& private_key_filename, const std::map<int, std::string>& public_key_files_by_id) :
    my_private_key(load_private_key(private_key_filename)),
    my_rsa_signer(my_private_key),
    my_rsa_encryptor(my_private_key),
    my_rsa_decryptor(my_private_key) {

    for(const auto& id_filename_pair : public_key_files_by_id) {
        CryptoPP::RSA::PublicKey key;
        CryptoPP::FileSource pub_key_file(id_filename_pair.second.c_str(), true);
        CryptoPP::ByteQueue temp_queue_2;
        pub_key_file.TransferAllTo(temp_queue_2);
        temp_queue_2.MessageEnd();
        key.Load(temp_queue_2);
        public_keys_by_id.emplace(id_filename_pair.first, std::move(key));
    }
}

std::shared_ptr<messaging::OverlayMessage> CryptoPPLibrary::rsa_encrypt(const std::shared_ptr<messaging::OverlayMessage>& message,
        int target_meter_id) {
    message->is_encrypted = true;
    if(message->body == nullptr) {
        return message;
    }
    //Serialize the body to make it a byte array to encrypt
    std::size_t body_bytes_size = mutils::bytes_size(*message->body);
    char body_bytes[body_bytes_size];
    mutils::to_bytes(*message->body, body_bytes);
    //Generate a fresh random AES key and set up the encryptor
    CryptoPP::SecByteBlock symmetric_key(CryptoPP::AES::DEFAULT_KEYLENGTH);
    rng.GenerateBlock(symmetric_key.data(), symmetric_key.size());
    byte iv[CryptoPP::AES::BLOCKSIZE];
    rng.GenerateBlock(iv, CryptoPP::AES::BLOCKSIZE);
    CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption aes_encryptor;
    aes_encryptor.SetKeyWithIV(symmetric_key, symmetric_key.size(), iv);

    CryptoPP::RSAES_OAEP_SHA_Encryptor target_rsa_encryptor(public_keys_by_id.at(target_meter_id));
    //Ugh, I guess I have to copy the key and IV into a new array in order to make them a single source for RSA encryption
    CryptoPP::SecByteBlock key_plus_iv = symmetric_key + CryptoPP::SecByteBlock(iv, CryptoPP::AES::BLOCKSIZE);
    //The encrypted body will first contain the RSA-encrypted key, then the AES-encrypted message body
    std::string encrypted_body_string;
    CryptoPP::ArraySource keystream(key_plus_iv.data(), key_plus_iv.size(), true,
            new CryptoPP::PK_EncryptorFilter(rng, target_rsa_encryptor,
                    new CryptoPP::StringSink(encrypted_body_string)));
    assert(encrypted_body_string.size() == target_rsa_encryptor.FixedCiphertextLength());
    CryptoPP::ArraySource stream((unsigned char*) body_bytes, body_bytes_size, true,
            new CryptoPP::StreamTransformationFilter(aes_encryptor,
                    new CryptoPP::StringSink(encrypted_body_string)));

    message->body = std::make_shared<messaging::StringBody>(encrypted_body_string);
    return message;
}

std::shared_ptr<messaging::OverlayMessage> CryptoPPLibrary::rsa_decrypt(const std::shared_ptr<messaging::OverlayMessage>& message) {
    message->is_encrypted = false;
    if(message->body == nullptr) {
        return message;
    }
    std::shared_ptr<messaging::StringBody> body_string = std::static_pointer_cast<messaging::StringBody>(message->body);
    CryptoPP::SecByteBlock key_plus_iv(CryptoPP::AES::DEFAULT_KEYLENGTH + CryptoPP::AES::BLOCKSIZE);
    //Decrypt exactly FixedCipherTextLength() bytes from the beginning of the body, to get the key+IV data block
    my_rsa_decryptor.Decrypt(rng, (byte*) body_string->c_str(), my_rsa_decryptor.FixedCiphertextLength(), key_plus_iv);
    CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption aes_decryptor;
    //Give SetKeyWithIV a pointer to the second part of the decrypted array, which should contain the IV
    aes_decryptor.SetKeyWithIV(key_plus_iv, CryptoPP::AES::DEFAULT_KEYLENGTH,
            key_plus_iv.data() + CryptoPP::AES::DEFAULT_KEYLENGTH);
    //Give the AES decryptor filter an "array source" pointing into the StringBody past the key+IV ciphertext
    std::string body_bytes;
    CryptoPP::ArraySource stream((byte*)(body_string->c_str() + my_rsa_decryptor.FixedCiphertextLength()),
            body_string->size() - my_rsa_decryptor.FixedCiphertextLength(), true,
            new CryptoPP::StreamTransformationFilter(aes_decryptor,
                    new CryptoPP::StringSink(body_bytes)));
    //Even though it's a "string," body_bytes is actually a serialized MessageBody
    message->body = mutils::from_bytes<messaging::MessageBody>(nullptr, body_bytes.data());
    return message;
}

std::shared_ptr<messaging::StringBody> CryptoPPLibrary::rsa_sign_encrypted(const std::shared_ptr<messaging::StringBody>& encrypted_message) {
    assert(false);
    return nullptr;
}

void CryptoPPLibrary::rsa_decrypt_signature(const std::shared_ptr<std::string>& blinded_signature, SignatureArray& signature) {
    assert(false);
}

std::shared_ptr<messaging::StringBody> CryptoPPLibrary::rsa_encrypt(const std::shared_ptr<messaging::ValueTuple>& value, const int target_meter_id) {
    assert(false);
    return nullptr;
}

void CryptoPPLibrary::rsa_sign(const messaging::ValueContribution& value, SignatureArray& signature) {
    std::size_t value_bytes_length = mutils::bytes_size(value);
    char value_bytes[value_bytes_length];
    mutils::to_bytes(value, value_bytes);
    my_rsa_signer.SignMessage(rng, (const byte*) value_bytes, value_bytes_length, signature.data());
}

bool CryptoPPLibrary::rsa_verify(const messaging::ValueContribution& value, const SignatureArray& signature, const int signer_meter_id) {
    assert(false);
    return false;
}

void CryptoPPLibrary::rsa_sign(const messaging::SignedValue& value, SignatureArray& signature) {
    assert(false);
}

bool CryptoPPLibrary::rsa_verify(const messaging::SignedValue& value, const SignatureArray& signature, const int signer_meter_id) {
    assert(false);
    return false;
}

} //namespace util
} //namespace pddm


