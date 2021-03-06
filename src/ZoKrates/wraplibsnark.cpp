/**
 * @file wraplibsnark.cpp
 * @author Jacob Eberhardt <jacob.eberhardt@tu-berlin.de
 * @author Dennis Kuhnert <dennis.kuhnert@campus.tu-berlin.de>
 * @date 2017
 */

#pragma once

#include <fstream>
#include <iostream>
#include <cassert>
#include <iomanip>

// contains definition of alt_bn128 ec public parameters
//#include "libsnark/libsnark/algebra/curves/alt_bn128/alt_bn128_pp.hpp"
#include "libff/algebra/curves/alt_bn128/alt_bn128_pp.hpp"
// contains required interfaces and types (keypair, proof, generator, prover, verifier)
#include <libsnark/zk_proof_systems/ppzksnark/r1cs_ppzksnark/r1cs_ppzksnark.hpp>
#include "r1cs_gg_ppzksnark_zok/r1cs_gg_ppzksnark_zok.hpp"

typedef long integer_coeff_t;

using namespace std;
using namespace libsnark;

// conversion byte[32] <-> libsnark bigint.
libff::bigint<libff::alt_bn128_r_limbs> libsnarkBigintFromBytes(const uint8_t* _x)
{
  libff::bigint<libff::alt_bn128_r_limbs> x;

  for (unsigned i = 0; i < 4; i++) {
    for (unsigned j = 0; j < 8; j++) {
      x.data[3 - i] |= uint64_t(_x[i * 8 + j]) << (8 * (7-j));
    }
  }
  return x;
}

std::string HexStringFromLibsnarkBigint(libff::bigint<libff::alt_bn128_r_limbs> _x){
    mpz_t value;
    ::mpz_init(value);

    _x.to_mpz(value);
    char *value_out_hex = mpz_get_str(NULL, 16, value);

    std::string str(value_out_hex);

    ::mpz_clear(value);
    ::free(value_out_hex);

    return str;
}

std::string outputPointG1AffineAsHex(libff::alt_bn128_G1 _p)
{
        libff::alt_bn128_G1 aff = _p;
        aff.to_affine_coordinates();
        //std::stringstream ss; 
        //ss << "0x"  << aff.X.as_bigint() << "," << aff.Y.as_bigint() << "," << aff.Z.as_bigint();

        return       "\"0x" + 
               HexStringFromLibsnarkBigint(aff.X.as_bigint()) +
                "\", \"0x"+
                HexStringFromLibsnarkBigint(aff.Y.as_bigint()) +
                "\""; 
}

std::string outputPointG1AffineAsInt(libff::alt_bn128_G1 _p)
{
        libff::alt_bn128_G1 aff = _p;
        aff.to_affine_coordinates();
        std::stringstream ss; 
        ss << ""  << aff.X.as_bigint() << "," << aff.Y.as_bigint() << "," << aff.Z.as_bigint();
        return ss.str();
}


std::string outputPointG2AffineAsHex(libff::alt_bn128_G2 _p)
{
        libff::alt_bn128_G2 aff = _p;

        if (aff.Z.c0.as_bigint() != "0" && aff.Z.c1.as_bigint() != "0" ) {
            aff.to_affine_coordinates();
        }
        return "[\"0x" +
                HexStringFromLibsnarkBigint(aff.X.c1.as_bigint()) + "\", \"0x" +
                HexStringFromLibsnarkBigint(aff.X.c0.as_bigint()) + "\"],\n [\"0x" + 
                HexStringFromLibsnarkBigint(aff.Y.c1.as_bigint()) + "\", \"0x" +
                HexStringFromLibsnarkBigint(aff.Y.c0.as_bigint()) + "\"]"; 
}
std::string outputPointG2AffineAsInt(libff::alt_bn128_G2 _p)
{
        libff::alt_bn128_G2 aff = _p;
        if (aff.Z.c0.as_bigint() != "0" && aff.Z.c1.as_bigint() != "0" ) {
            aff.to_affine_coordinates();
        }
        std::stringstream ss;
        ss << ""  << aff.X.c1.as_bigint() << "," << aff.X.c0.as_bigint() << "," << aff.Y.c1.as_bigint() << "," << aff.Y.c0.as_bigint() << "," << aff.Z.c1.as_bigint() << "," <<aff.Z.c0.as_bigint()  ;

        return ss.str();
}


// keypair generateKeypair(constraints)
r1cs_gg_ppzksnark_zok_keypair<libff::alt_bn128_pp> generateKeypair(const r1cs_ppzksnark_constraint_system<libff::alt_bn128_pp> &cs){
  // from r1cs_ppzksnark.hpp
  return r1cs_gg_ppzksnark_zok_generator<libff::alt_bn128_pp>(cs);
}

template<typename T>
void writeToFile(std::string path, T& obj) {
    std::stringstream ss;
    ss << obj;
    std::ofstream fh;
    fh.open(path, std::ios::binary);
    ss.rdbuf()->pubseekpos(0, std::ios_base::out);
    fh << ss.rdbuf();
    fh.flush();
    fh.close();
}

template<typename T>
T loadFromFile(std::string path) {
    std::stringstream ss;
    std::ifstream fh(path, std::ios::binary);

    // TODO: more useful error if file not found
    assert(fh.is_open());

    ss << fh.rdbuf();
    fh.close();

    ss.rdbuf()->pubseekpos(0, std::ios_base::in);

    T obj;
    ss >> obj;

    return obj;
}

void serializeProvingKeyToFile(r1cs_ppzksnark_proving_key<libff::alt_bn128_pp> pk, const char* pk_path){
  writeToFile(pk_path, pk);
}

r1cs_ppzksnark_proving_key<libff::alt_bn128_pp> deserializeProvingKeyFromFile(const char* pk_path){
  return loadFromFile<r1cs_ppzksnark_proving_key<libff::alt_bn128_pp>>(pk_path);
}

void serializeVerificationKeyToFile(r1cs_ppzksnark_verification_key<libff::alt_bn128_pp> vk, const char* vk_path){
  std::stringstream ss;

  unsigned icLength = vk.encoded_IC_query.rest.indices.size() + 1;

  ss << "\t\tvk.A = " << outputPointG2AffineAsHex(vk.alphaA_g2) << endl;
  ss << "\t\tvk.B = " << outputPointG1AffineAsHex(vk.alphaB_g1) << endl;
  ss << "\t\tvk.C = " << outputPointG2AffineAsHex(vk.alphaC_g2) << endl;
  ss << "\t\tvk.gamma = " << outputPointG2AffineAsHex(vk.gamma_g2) << endl;
  ss << "\t\tvk.gammaBeta1 = " << outputPointG1AffineAsHex(vk.gamma_beta_g1) << endl;
  ss << "\t\tvk.gammaBeta2 = " << outputPointG2AffineAsHex(vk.gamma_beta_g2) << endl;
  ss << "\t\tvk.Z = " << outputPointG2AffineAsHex(vk.rC_Z_g2) << endl;
  ss << "\t\tvk.IC.len() = " << icLength << endl;
  ss << "\t\tvk.IC[0] = " << outputPointG1AffineAsHex(vk.encoded_IC_query.first) << endl;
  for (size_t i = 1; i < icLength; ++i)
  {
                  auto vkICi = outputPointG1AffineAsHex(vk.encoded_IC_query.rest.values[i - 1]);
                  ss << "\t\tvk.IC[" << i << "] = " << vkICi << endl;
  }

  std::ofstream fh;
  fh.open(vk_path, std::ios::binary);
  ss.rdbuf()->pubseekpos(0, std::ios_base::out);
  fh << ss.rdbuf();
  fh.flush();
  fh.close();
}

// compliant with solidty verification example
void exportVerificationKey(r1cs_ppzksnark_keypair<libff::alt_bn128_pp> keypair){
        unsigned icLength = keypair.vk.encoded_IC_query.rest.indices.size() + 1;

        cout << "\tVerification key in Solidity compliant format:{" << endl;
        cout << "\t\tvk.A = Pairing.G2Point(" << outputPointG2AffineAsHex(keypair.vk.alphaA_g2) << ");" << endl;
        cout << "\t\tvk.B = Pairing.G1Point(" << outputPointG1AffineAsHex(keypair.vk.alphaB_g1) << ");" << endl;
        cout << "\t\tvk.C = Pairing.G2Point(" << outputPointG2AffineAsHex(keypair.vk.alphaC_g2) << ");" << endl;
        cout << "\t\tvk.gamma = Pairing.G2Point(" << outputPointG2AffineAsHex(keypair.vk.gamma_g2) << ");" << endl;
        cout << "\t\tvk.gammaBeta1 = Pairing.G1Point(" << outputPointG1AffineAsHex(keypair.vk.gamma_beta_g1) << ");" << endl;
        cout << "\t\tvk.gammaBeta2 = Pairing.G2Point(" << outputPointG2AffineAsHex(keypair.vk.gamma_beta_g2) << ");" << endl;
        cout << "\t\tvk.Z = Pairing.G2Point(" << outputPointG2AffineAsHex(keypair.vk.rC_Z_g2) << ");" << endl;
        cout << "\t\tvk.IC = new Pairing.G1Point[](" << icLength << ");" << endl;
        cout << "\t\tvk.IC[0] = Pairing.G1Point(" << outputPointG1AffineAsHex(keypair.vk.encoded_IC_query.first) << ");" << endl;
        for (size_t i = 1; i < icLength; ++i)
        {
                auto vkICi = outputPointG1AffineAsHex(keypair.vk.encoded_IC_query.rest.values[i - 1]);
                cout << "\t\tvk.IC[" << i << "] = Pairing.G1Point(" << vkICi << ");" << endl;
        }
        cout << "\t\t}" << endl;

}

// compliant with solidty verification example
/*
void exportInput(r1cs_primary_input<libff::alt_bn128_pp> input){
        cout << "\tInput in Solidity compliant format:{" << endl;
        for (size_t i = 0; i < input.size(); ++i)
        {
                cout << "\t\tinput[" << i << "] = " << HexStringFromLibsnarkBigint(input[i].as_bigint()) << ";" << endl;
        }
        cout << "\t\t}" << endl;
} */


void printProof(r1cs_ppzksnark_proof<libff::alt_bn128_pp> proof){
                cout << "Proof:"<< endl;
                cout << "proof.A = Pairing.G1Point(" << outputPointG1AffineAsHex(proof.g_A.g)<< ");" << endl;
                cout << "proof.A_p = Pairing.G1Point(" << outputPointG1AffineAsHex(proof.g_A.h)<< ");" << endl;
                cout << "proof.B = Pairing.G2Point(" << outputPointG2AffineAsHex(proof.g_B.g)<< ");" << endl;
                cout << "proof.B_p = Pairing.G1Point(" << outputPointG1AffineAsHex(proof.g_B.h)<<");" << endl;
                cout << "proof.C = Pairing.G1Point(" << outputPointG1AffineAsHex(proof.g_C.g)<< ");" << endl;
                cout << "proof.C_p = Pairing.G1Point(" << outputPointG1AffineAsHex(proof.g_C.h)<<");" << endl;
                cout << "proof.H = Pairing.G1Point(" << outputPointG1AffineAsHex(proof.g_H)<<");"<< endl;
                cout << "proof.K = Pairing.G1Point(" << outputPointG1AffineAsHex(proof.g_K)<<");"<< endl;
}

/*bool _setup(const uint8_t* A, const uint8_t* B, const uint8_t* C, int constraints, int variables, int inputs, const char* pk_path, const char* vk_path)
{
  //libsnark::inhibit_profiling_info = true;
  //libsnark::inhibit_profiling_counters = true;

  //initialize curve parameters
  libff::alt_bn128_pp::init_public_params();

  r1cs_constraint_system<libff::alt_bn128_pp> cs;
  cs = createConstraintSystem(A, B ,C , constraints, variables, inputs);

  assert(cs.num_variables() >= inputs);
  assert(cs.num_inputs() == inputs);
  assert(cs.num_constraints() == constraints);

  // create keypair
  r1cs_ppzksnark_keypair<alt_bn128_pp> keypair = r1cs_ppzksnark_generator<alt_bn128_pp>(cs);

  // Export vk and pk to files
  serializeProvingKeyToFile(keypair.pk, pk_path);
  serializeVerificationKeyToFile(keypair.vk, vk_path);

  // Print VerificationKey in Solidity compatible format
  exportVerificationKey(keypair);

  return true;
}*/
/*
bool _generate_proof(const char* pk_path, const uint8_t* public_inputs, int public_inputs_length, const uint8_t* private_inputs, int private_inputs_length)
{
//  libsnark::inhibit_profiling_info = true;
//  libsnark::inhibit_profiling_counters = true;

  //initialize curve parameters
  libff::alt_bn128_pp::init_public_params();
  r1cs_ppzksnark_proving_key<libff::alt_bn128_pp> pk = deserializeProvingKeyFromFile(pk_path);

  // assign variables based on witness values, excludes ~one
  r1cs_variable_assignment<libff::alt_bn128_pp> full_variable_assignment;
  for (int i = 1; i < public_inputs_length; i++) {
    full_variable_assignment.push_back(libff::alt_bn128_pp(libsnarkBigintFromBytes(public_inputs + i*32)));
  }
  for (int i = 0; i < private_inputs_length; i++) {
    full_variable_assignment.push_back(<libff::alt_bn128_pp>(libsnarkBigintFromBytes(private_inputs + i*32)));
  }

  // split up variables into primary and auxiliary inputs. Does *NOT* include the constant 1
  // Public variables belong to primary input, private variables are auxiliary input.
  r1cs_primary_input<libff::alt_bn128_pp> primary_input(full_variable_assignment.begin(), full_variable_assignment.begin() + public_inputs_length-1);
  r1cs_primary_input<libff::alt_bn128_pp> auxiliary_input(full_variable_assignment.begin() + public_inputs_length-1, full_variable_assignment.end());

  // for debugging
  // cout << "full variable assignment:"<< endl << full_variable_assignment;
  // cout << "primary input:"<< endl << primary_input;
  // cout << "auxiliary input:"<< endl << auxiliary_input;

  // Proof Generation
  r1cs_ppzksnark_proof<alt_bn128_pp> proof = r1cs_ppzksnark_prover<alt_bn128_pp>(pk, primary_input, auxiliary_input);

  // print proof
  printProof(proof);
  // TODO? print inputs

  return true;
} */
