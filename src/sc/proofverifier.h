#ifndef _SC_PROOF_VERIFIER_H
#define _SC_PROOF_VERIFIER_H

#include <map>

#include <boost/variant.hpp>

#include "amount.h"
#include "primitives/certificate.h"
#include "primitives/transaction.h"
#include "sc/sidechaintypes.h"

class CSidechain;
class CScCertificate;
class uint256;
class CCoinsViewCache;

/**
 * The enumeration of possible results of the proof verifier for any proof processed.
 */
enum class ProofVerificationResult
{
    Unknown,    /**< The result of the batch verification is unknown. */
    Failed,     /**< The proof verification failed. */
    Passed      /**< The proof verification passed. */
};

std::string ProofVerificationResultToString(ProofVerificationResult res);

/**
 * @brief A base structure for generic inputs of the proof verifier.
 */
struct CBaseProofVerifierInput
{
    uint32_t proofId;                               /**< A unique number identifying the proof. */
    CScProof proof;                                 /**< The proof to be verified. */
    CScVKey verificationKey;                        /**< The key to be used for the verification. */
};

/**
 * @brief A structure that includes all the arguments needed for verifying the proof of a CSW input.
 */
struct CCswProofVerifierInput : CBaseProofVerifierInput
{
    CFieldElement ceasingCumScTxCommTree;
    CFieldElement certDataHash;
    CAmount nValue;
    CFieldElement nullifier;
    uint160 pubKeyHash;
    uint256 scId;
};

/**
 * @brief A structure that includes all the arguments needed for verifying the proof of a certificate.
 */
struct CCertProofVerifierInput : CBaseProofVerifierInput
{
    uint256 certHash;
    CFieldElement constant;
    uint32_t epochNumber;
    uint64_t quality;
    std::vector<backward_transfer_t> bt_list;
    std::vector<CFieldElement> vCustomFields;
    CFieldElement endEpochCumScTxCommTreeRoot;
    uint64_t mainchainBackwardTransferRequestScFee;
    uint64_t forwardTransferScFee;
};

/**
 * @brief The base structure for items added to the queue of the proof verifier.
 */
struct CProofVerifierItem
{
    uint256 txHash;                                                     /**< The hash of the transaction/certificate whose proof(s) have to be verified. */
    std::shared_ptr<CTransactionBase> parentPtr;                        /**< The parent (Transaction or Certificate) that owns the item (CSW input or certificate itself). */
    CNode* node;                                                        /**< The node that sent the parent (Transaction or Certiticate). */
    ProofVerificationResult result;                                     /**< The overall result of the proof(s) verification for the transaction/certificate. */
    boost::optional<CCertProofVerifierInput> certInput;                 /**< Certificate data to provide the proof verifier with. */
    boost::optional<std::vector<CCswProofVerifierInput>> cswInputs;     /**< CSW outputs data to provide the proof verifier with. */
};

/* A verifier that is able to verify different kind of ScProof(s) */
class CScProofVerifier
{
public:

    /**
     * @brief The types of verification that can be requested to the proof verifier.
     */
    enum class Verification
    {
        Strict,     /**< Perform normal verification. */
        Loose       /**< Skip verification. */
    };

    static CCertProofVerifierInput CertificateToVerifierItem(const CScCertificate& certificate, const Sidechain::ScFixedParameters& scFixedParams, CNode* pfrom);
    static CCswProofVerifierInput CswInputToVerifierItem(const CTxCeasedSidechainWithdrawalInput& cswInput, const CTransaction* cswTransaction, const Sidechain::ScFixedParameters& scFixedParams, CNode* pfrom);

    CScProofVerifier(Verification mode) :
    verificationMode(mode)
    {
    }
    ~CScProofVerifier() = default;

    // CScProofVerifier should never be copied
    CScProofVerifier(const CScProofVerifier&) = delete;
    CScProofVerifier& operator=(const CScProofVerifier&) = delete;

    virtual void LoadDataForCertVerification(const CCoinsViewCache& view, const CScCertificate& scCert, CNode* pfrom = nullptr);

    virtual void LoadDataForCswVerification(const CCoinsViewCache& view, const CTransaction& scTx, CNode* pfrom = nullptr);
    bool BatchVerify();

protected:

    bool BatchVerifyInternal(std::map</* Cert or Tx hash */ uint256, CProofVerifierItem>& proofs);
    void NormalVerify(std::map</* Cert or Tx hash */ uint256, CProofVerifierItem>& proofs);
    ProofVerificationResult NormalVerifyCertificate(CCertProofVerifierInput input) const;
    ProofVerificationResult NormalVerifyCsw(std::vector<CCswProofVerifierInput> cswInputs) const;

    std::map</* Cert or Tx hash */ uint256, CProofVerifierItem> proofQueue;   /**< The queue of proofs to be verified. */

private:

    static std::atomic<uint32_t> proofIdCounter;   /**< The counter used to get a unique ID for proofs. */

    const Verification verificationMode;    /**< The type of verification to be performed by this instance of proof verifier. */
};

#endif // _SC_PROOF_VERIFIER_H
