// Copyright (c) 2018 HarryR
// License: LGPL-3.0+

#include "sha256/sha256_full_gadget.cpp"
#include "sha256/utils.cpp"

#include <libff/algebra/fields/field_utils.hpp>

#include <openssl/sha.h>


template<typename FieldT>
pb_variable_array<FieldT> pb_variable_array_allocate( protoboard<FieldT> &in_pb, size_t n, const std::string &annotation_prefix )
{
    pb_variable_array<FieldT> res;
    res.allocate(in_pb, n, annotation_prefix);
    return res;
}


/**
* Verify that SHA256(private<512bit_block>) == public<output>
*/
template<typename FieldT>
class mod_hashpreimage : public gadget<FieldT>
{
public:
    static const size_t SHA256_digest_size_bytes = SHA256_digest_size / 8;

    static const size_t SHA256_block_size_bytes = SHA256_block_size / 8;

    const size_t input_size_in_bits = SHA256_digest_size;

    const size_t input_size_in_fields;

    const pb_variable_array<FieldT> input_as_field_elements;

    digest_variable<FieldT> input_digest;

    const pb_variable_array<FieldT> input_as_bits;

    multipacking_gadget<FieldT> unpacker;

    block_variable<FieldT> input_block;

    digest_variable<FieldT> output;

    sha256_full_gadget_512<FieldT> full_hasher;


    mod_hashpreimage(
        protoboard<FieldT> &in_pb,
        const std::string &annotation_prefix
    ) :
        gadget<FieldT>(in_pb, annotation_prefix),

        // number of field packed elements as input
        input_size_in_fields( libff::div_ceil(input_size_in_bits, FieldT::capacity()) ),

        // packed input, given to prover/verifier
        input_as_field_elements( pb_variable_array_allocate<FieldT>(in_pb, input_size_in_fields, FMT(annotation_prefix, " input_as_field_elements")) ),

        // public input digest, must match output
        input_digest(in_pb, SHA256_digest_size, FMT(annotation_prefix, " input_digest")),

        // unpacked input bits, mapped to the input digest
        input_as_bits(input_digest.bits.begin(), input_digest.bits.end()),

        // unpack from field elements (packed bits)
        unpacker(in_pb, input_as_bits, input_as_field_elements, FieldT::capacity(), "unpacker"),

        // private input block for hashing, 512 bits
        input_block(in_pb, SHA256_block_size, FMT(annotation_prefix, " input_block")),

        // output digest, 256 bits
        output(in_pb, SHA256_digest_size, FMT(annotation_prefix, " output")),

        // hasher, HASH(input_block) -> output
        full_hasher(in_pb, input_block, output, FMT(annotation_prefix, " full_hasher"))
    {
        in_pb.set_input_sizes( input_size_in_fields );
    }


    void generate_r1cs_constraints()
    {
        unpacker.generate_r1cs_constraints(true);

        full_hasher.generate_r1cs_constraints();
    }


    void generate_r1cs_witness(
        const libff::bit_vector &in_block,
        const libff::bit_vector &in_expected_bv
    ) {
        assert( in_block.size() == SHA256_block_size );

        assert( in_expected_bv.size() == SHA256_digest_size );

        input_block.generate_r1cs_witness(in_block);

        full_hasher.generate_r1cs_witness();

        input_digest.generate_r1cs_witness(in_expected_bv);

        output.generate_r1cs_witness(in_expected_bv);

        unpacker.generate_r1cs_witness_from_bits();
    }


    /**
    * Given input bytes of SHA256 block size, generate the witness for the expected output
    */
    void generate_r1cs_witness(
        const uint8_t in_bytes[SHA256_block_size_bytes]
    ) {
        SHA256_CTX ctx;
        uint8_t output_digest[SHA256_digest_size_bytes];

        SHA256_Init(&ctx);
        SHA256_Update(&ctx, in_bytes, SHA256_block_size_bytes);
        SHA256_Final(output_digest, &ctx);

        const auto input_bv = bytes_to_bv(in_bytes, SHA256_block_size_bytes);

        const auto output_bv = bytes_to_bv(output_digest, SHA256_digest_size_bytes);

        this->generate_r1cs_witness( input_bv, output_bv );
    }


    static r1cs_primary_input<FieldT> make_primary_input(const libff::bit_vector &in_block_bv)
    {
        assert( in_block_bv.size() == SHA256_block_size );

        return libff::pack_bit_vector_into_field_element_vector<FieldT>(in_block_bv);
    }
};
