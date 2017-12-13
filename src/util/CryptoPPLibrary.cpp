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

std::function<CryptoPPLibrary (MeterClient&)> crypto_library_builder(const std::string& private_key_filename,
        const std::map<int, std::string>& public_key_files_by_id) {
    return [private_key_filename, public_key_files_by_id](MeterClient&) {
        return CryptoPPLibrary(private_key_filename, public_key_files_by_id);
    };
}

std::function<CryptoPPLibrary (UtilityClient&)> crypto_library_builder_utility(const std::string& private_key_filename,
        const std::map<int, std::string>& public_key_files_by_id) {
    return [private_key_filename, public_key_files_by_id](UtilityClient&) {
        return CryptoPPLibrary(private_key_filename, public_key_files_by_id);
    };
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
    //Note that StringBody behaves exactly like a string, so the StringSink should be able to write to it
    std::shared_ptr<messaging::StringBody> encrypted_body_string = std::make_shared<messaging::StringBody>();
    CryptoPP::ArraySource key_stream(key_plus_iv.data(), key_plus_iv.size(), true,
            new CryptoPP::PK_EncryptorFilter(rng, target_rsa_encryptor,
                    new CryptoPP::StringSink(*encrypted_body_string)));
    assert(encrypted_body_string->size() == target_rsa_encryptor.FixedCiphertextLength());
    CryptoPP::ArraySource message_stream((unsigned char*) body_bytes, body_bytes_size, true,
            new CryptoPP::StreamTransformationFilter(aes_encryptor,
                    new CryptoPP::StringSink(*encrypted_body_string)));

    message->body = encrypted_body_string;
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

std::shared_ptr<messaging::StringBody> CryptoPPLibrary::rsa_blind(const std::shared_ptr<messaging::ValueTuple>& value) {
    std::size_t bytes_size = mutils::bytes_size(*value);
    char value_bytes[bytes_size];
    mutils::to_bytes(*value, value_bytes);
    CryptoPP::Integer value_as_int((const byte*) value_bytes, bytes_size);
    CryptoPP::ModularArithmetic utility_modulus_operator(public_keys_by_id.at(-1).GetModulus());
    //Try to pick a random blind "r" that has an inverse modulo N
    CryptoPP::Integer r, rInv;
    do {
        r.Randomize(rng, CryptoPP::Integer::One(), utility_modulus_operator.GetModulus() - CryptoPP::Integer::One());
        rInv = utility_modulus_operator.MultiplicativeInverse(r);
    } while (rInv.IsZero());
    last_blind_inverse = rInv;
    //Raise r to the utility's public key
    CryptoPP::Integer r_to_e = public_keys_by_id.at(-1).ApplyFunction(r);
    CryptoPP::Integer blinded_int = utility_modulus_operator.Multiply(value_as_int, r_to_e);
    //Now I want to store this "integer" as bytes so I can put it in a StringBody. Is this how to do it?? Or will "encode" transform it irreversably?
    std::shared_ptr<messaging::StringBody> blinded_int_bytes = std::make_shared<messaging::StringBody>(blinded_int.MinEncodedSize(), ' ');
    blinded_int.Encode((byte*) blinded_int_bytes->data(), blinded_int_bytes->size());
    return blinded_int_bytes;
}

std::shared_ptr<messaging::StringBody> CryptoPPLibrary::rsa_sign_blinded(const std::shared_ptr<messaging::StringBody>& blinded_message) {
    std::shared_ptr<messaging::StringBody> signature_message = std::make_shared<messaging::StringBody>();
    //This will do it "correctly," but I actually need to do an unpadded, "dangerous" signature for the blinding to work
//    CryptoPP::StringSource sign_stream(*blinded_message, true,
//            new CryptoPP::SignerFilter(rng, my_rsa_signer,
//                    new CryptoPP::StringSink(*signature_message)));

    CryptoPP::Integer blind_message_as_int((const byte*) blinded_message->data(), blinded_message->size());
    //CalculateInverse raises the input to the private exponent
    CryptoPP::Integer signed_blind_message = my_private_key.CalculateInverse(rng, blind_message_as_int);
    signature_message->resize(signed_blind_message.MinEncodedSize());
    signed_blind_message.Encode((byte*) signature_message->data(), signature_message->size());
    return signature_message;
}

void CryptoPPLibrary::rsa_unblind_signature(const std::shared_ptr<std::string>& blinded_signature, SignatureArray& signature) {
    CryptoPP::Integer blinded_sig_as_int((const byte*) blinded_signature->data(), blinded_signature->size());
    CryptoPP::ModularArithmetic utility_modulus_operator(public_keys_by_id.at(-1).GetModulus());
    CryptoPP::Integer sig_as_int = utility_modulus_operator.Multiply(blinded_sig_as_int, last_blind_inverse);
    //This had better work...unless this "encoding" of an "integer" doesn't do what I think it does
    assert(sig_as_int.MinEncodedSize() <= signature.size());
    sig_as_int.Encode((byte*) signature.data(), signature.size());
}

std::shared_ptr<messaging::StringBody> CryptoPPLibrary::rsa_encrypt(const std::shared_ptr<messaging::ValueTuple>& value, const int target_meter_id) {
    std::size_t bytes_size = mutils::bytes_size(*value);
    char value_bytes[bytes_size];
    mutils::to_bytes(*value, value_bytes);
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
    //The encrypted message body will first contain the RSA-encrypted key, then the AES-encrypted ValueTuple
    std::shared_ptr<messaging::StringBody> encrypted_body_string = std::make_shared<messaging::StringBody>();
    CryptoPP::ArraySource key_stream(key_plus_iv.data(), key_plus_iv.size(), true,
            new CryptoPP::PK_EncryptorFilter(rng, target_rsa_encryptor,
                    new CryptoPP::StringSink(*encrypted_body_string)));
    assert(encrypted_body_string->size() == target_rsa_encryptor.FixedCiphertextLength());
    CryptoPP::ArraySource message_stream((unsigned char*) value_bytes, bytes_size, true,
            new CryptoPP::StreamTransformationFilter(aes_encryptor,
                    new CryptoPP::StringSink(*encrypted_body_string)));
    return encrypted_body_string;
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


