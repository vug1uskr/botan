/*
* (C) 2014,2015 Jack Lloyd
* (C) 2017 René Korthaus, Rohde & Schwarz Cybersecurity
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include "tests.h"

#include "test_rng.h"

#if defined(BOTAN_HAS_ECDSA)
   #include "test_pubkey.h"
   #include <botan/ecdsa.h>
   #include <botan/oids.h>
#endif

namespace Botan_Tests {

namespace {

#if defined(BOTAN_HAS_ECDSA)

class ECDSA_Signature_KAT_Tests final : public PK_Signature_Generation_Test
   {
   public:
      ECDSA_Signature_KAT_Tests() : PK_Signature_Generation_Test(
            "ECDSA",
#if defined(BOTAN_HAS_RFC6979_GENERATOR)
            "pubkey/ecdsa_rfc6979.vec",
            "Group,X,Hash,Msg,Signature") {}
#else
            "pubkey/ecdsa_prob.vec",
            "Group,X,Hash,Msg,Nonce,Signature") {}
#endif

      bool clear_between_callbacks() const override
         {
         return false;
         }

      std::unique_ptr<Botan::Private_Key> load_private_key(const VarMap& vars) override
         {
         const std::string group_id = get_req_str(vars, "Group");
         const BigInt x = get_req_bn(vars, "X");
         Botan::EC_Group group(Botan::OIDS::lookup(group_id));

         std::unique_ptr<Botan::Private_Key> key(new Botan::ECDSA_PrivateKey(Test::rng(), group, x));
         return key;
         }

      std::string default_padding(const VarMap& vars) const override
         {
         const std::string hash = get_req_str(vars, "Hash");
         if(hash.substr(0,3) == "Raw")
            return hash;
         return "EMSA1(" + hash + ")";
         }

#if !defined(BOTAN_HAS_RFC6979)
      Botan::RandomNumberGenerator* test_rng(const std::vector<uint8_t>& nonce) const override
         {
         // probabilistic ecdsa signature generation extracts more random than just the nonce,
         // but the nonce is extracted first
         return new Fixed_Output_Position_RNG(nonce, 1);
         }
#endif
   };

class ECDSA_Keygen_Tests final : public PK_Key_Generation_Test
   {
   public:
      std::vector<std::string> keygen_params() const override
         {
         return { "secp256r1", "secp384r1", "secp521r1", "frp256v1" };
         }
      std::string algo_name() const override
         {
         return "ECDSA";
         }
   };

class ECDSA_Invalid_Key_Tests final : public Text_Based_Test
   {
   public:
      ECDSA_Invalid_Key_Tests() :
         Text_Based_Test("pubkey/ecdsa_invalid.vec", "Group,InvalidKeyX,InvalidKeyY") {}

      bool clear_between_callbacks() const override
         {
         return false;
         }

      Test::Result run_one_test(const std::string&, const VarMap& vars) override
         {
         Test::Result result("ECDSA invalid keys");

         const std::string group_id = get_req_str(vars, "Group");
         Botan::EC_Group group(Botan::OIDS::lookup(group_id));
         const Botan::BigInt x = get_req_bn(vars, "InvalidKeyX");
         const Botan::BigInt y = get_req_bn(vars, "InvalidKeyY");

         std::unique_ptr<Botan::PointGFp> public_point;

         try
            {
            public_point.reset(new Botan::PointGFp(group.point(x, y)));
            }
         catch(Botan::Invalid_Argument&)
            {
            // PointGFp() performs a range check on x, y in [0, p−1],
            // which is also part of the EC public key checks, e.g.,
            // in NIST SP800-56A rev2, sec. 5.6.2.3.2
            result.test_success("public key fails check");
            return result;
            }

         std::unique_ptr<Botan::Public_Key> key(new Botan::ECDSA_PublicKey(group, *public_point));
         result.test_eq("public key fails check", key->check_key(Test::rng(), false), false);
         return result;
         }
   };

BOTAN_REGISTER_TEST("ecdsa_sign", ECDSA_Signature_KAT_Tests);
BOTAN_REGISTER_TEST("ecdsa_keygen", ECDSA_Keygen_Tests);
BOTAN_REGISTER_TEST("ecdsa_invalid", ECDSA_Invalid_Key_Tests);

#endif

}

}
