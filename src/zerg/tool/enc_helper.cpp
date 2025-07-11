#include <zerg/tool/enc_helper.h>
#include <cryptopp/modes.h>
#include <cryptopp/aes.h>
#include <cryptopp/filters.h>
#include <cryptopp/base64.h>

namespace zerg {
std::string encrypt(const std::string& input, const std::vector<uint8_t>& key, const std::vector<uint8_t>& iv) {
    std::string cipher;
    auto aes = CryptoPP::AES::Encryption(key.data(), key.size());
    auto aes_cbc = CryptoPP::CBC_Mode_ExternalCipher::Encryption(aes, iv.data());

    CryptoPP::StringSource ss(
        input,
        true,
        new CryptoPP::StreamTransformationFilter(
            aes_cbc,
            new CryptoPP::Base64Encoder(
                new CryptoPP::StringSink(cipher)
            )
        )
    );
    return cipher;
}

std::string decrypt(const std::string& cipher_text, const std::vector<uint8_t>& key, const std::vector<uint8_t>& iv) {
    std::string plain_text;
    auto aes = CryptoPP::AES::Decryption(key.data(), key.size());
    auto aes_cbc = CryptoPP::CBC_Mode_ExternalCipher::Decryption(aes, iv.data());
    CryptoPP::StringSource ss(
        cipher_text,
        true,
        new CryptoPP::Base64Decoder(
            new CryptoPP::StreamTransformationFilter(
                aes_cbc,
                new CryptoPP::StringSink(plain_text)
            )
        )
    );
    return plain_text;
}

static std::vector<uint8_t> prepare_ikey(const std::string& key) {
    constexpr int key_len = 32;
    std::vector<uint8_t> ikey(key_len, 0);
    size_t _len = key.size();
    if (_len > key_len) _len = key_len;
    memcpy(ikey.data(), key.data(), _len);
    return ikey;
}

std::string encrypt(const std::string& input, const std::string& key) {
    std::vector<uint8_t> ikey = prepare_ikey(key);
    return encrypt(input, ikey, ikey);
}
std::string decrypt(const std::string& cipher_text, const std::string& key) {
    std::vector<uint8_t> ikey = prepare_ikey(key);
    return decrypt(cipher_text, ikey, ikey);
}
}