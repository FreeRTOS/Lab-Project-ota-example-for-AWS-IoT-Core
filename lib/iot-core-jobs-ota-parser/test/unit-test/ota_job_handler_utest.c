/*
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License. See the LICENSE accompanying this file
 * for the specific language governing permissions and limitations under
 * the License.
 */

#include <stdbool.h>
#include <string.h>

#include "unity.h"

#include "mock_job_parser.h"
#include "ota_job_processor.h"

#define JOB_DOC_ID              "jobDocId"
#define JOB_DOC_ID_LEN          8U
#define AFR_OTA_DOCUMENT        "{\"afr_ota\":{\"files\":[{\"filesize\":123456789}]}}"
#define AFR_OTA_DOCUMENT_LENGTH ( sizeof( AFR_OTA_DOCUMENT ) - 1U )
#define MULTI_FILE_OTA_DOCUMENT                                    \
    "{\"afr_ota\":{\"files\":[{\"filesize\":1},{\"filesize\":2},{" \
    "\"filesize\":3}]}}"
#define MULTI_FILE_OTA_DOCUMENT_LENGTH \
    ( sizeof( MULTI_FILE_OTA_DOCUMENT ) - 1U )
#define TOO_MANY_FILES_OTA_DOCUMENT                                        \
    "{\"afr_ota\":{\"files\":[{\"filesize\":1},{\"filesize\":2},{"         \
    "\"filesize\":3},{\"filesize\":4},{\"filesize\":5},{\"filesize\":6},{" \
    "\"filesize\":7},{\"filesize\":8},{\"filesize\":9},{\"filesize\":10}]}}"
#define TOO_MANY_FILES_OTA_DOCUMENT_LENGTH \
    ( sizeof( TOO_MANY_FILES_OTA_DOCUMENT ) - 1U )
#define CUSTOM_DOCUMENT        "{\"custom_job\":\"test\"}"
#define CUSTOM_DOCUMENT_LENGTH ( sizeof( CUSTOM_DOCUMENT ) - 1U )

AfrOtaJobDocumentFields_t parsedFields;

/* ===========================   UNITY FIXTURES ============================ */

/* Called before each test method. */
void setUp()
{
    parsedFields.signature = "expectedSignature";
    parsedFields.signatureLen = strlen( "expectedSignature" );
    parsedFields.filepath = "expectedFilepath";
    parsedFields.filepathLen = strlen( "expectedFilepath" );
    parsedFields.certfile = "expectedCertfile";
    parsedFields.certfileLen = strlen( "expectedCertfile" );
    parsedFields.authScheme = "expectedAuthScheme";
    parsedFields.authSchemeLen = strlen( "expectedAuthScheme" );
    parsedFields.imageRef = "expectedImageRef";
    parsedFields.imageRefLen = strlen( "expectedImageRef" );
    parsedFields.fileId = UINT32_MAX;
    parsedFields.fileSize = UINT32_MAX;
    parsedFields.fileType = UINT32_MAX;
}

/* Called after each test method. */
void tearDown()
{
}

/* Called at the beginning of the whole suite. */
void suiteSetUp()
{
}

/* Called at the end of the whole suite. */
int suiteTearDown( int numFailures )
{
    return numFailures;
}

/*
 * NOTE: In production, the string fields would not be null-terminated strings,
 * however since we're mocking the return we can force them to be
 * null-terminated for easier validation.
 */
void verifyCallbackValues( AfrOtaJobDocumentFields_t * params )
{
    TEST_ASSERT_EQUAL_STRING( "expectedSignature", params->signature );
    TEST_ASSERT_EQUAL( strlen( "expectedSignature" ), params->signatureLen );
    TEST_ASSERT_EQUAL_STRING( "expectedFilepath", params->filepath );
    TEST_ASSERT_EQUAL( strlen( "expectedFilepath" ), params->filepathLen );
    TEST_ASSERT_EQUAL_STRING( "expectedCertfile", params->certfile );
    TEST_ASSERT_EQUAL( strlen( "expectedCertfile" ), params->certfileLen );
    TEST_ASSERT_EQUAL_STRING( "expectedAuthScheme", params->authScheme );
    TEST_ASSERT_EQUAL( strlen( "expectedAuthScheme" ), params->authSchemeLen );
    TEST_ASSERT_EQUAL_STRING( "expectedImageRef", params->imageRef );
    TEST_ASSERT_EQUAL( strlen( "expectedImageRef" ), params->imageRefLen );
    TEST_ASSERT_EQUAL( UINT32_MAX, params->fileId );
    TEST_ASSERT_EQUAL( UINT32_MAX, params->fileSize );
    TEST_ASSERT_EQUAL( UINT32_MAX, params->fileType );
}

static void expectPopulateJobDocWithFileIndex( const char * document,
                                               size_t docLength,
                                               int index )
{
    populateJobDocFields_ExpectAndReturn( document,
                                          docLength,
                                          index,
                                          NULL,
                                          true );
    populateJobDocFields_IgnoreArg_result();
    populateJobDocFields_ReturnThruPtr_result( &parsedFields );
}

/* ===============================   TESTS   =============================== */

void test_handleJobDoc_returnsTrue_whenIOTOtaJob( void )
{
    expectPopulateJobDocWithFileIndex( AFR_OTA_DOCUMENT,
                                       AFR_OTA_DOCUMENT_LENGTH,
                                       0 );

    bool result = otaParser_handleJobDoc( &verifyCallbackValues,
                                          JOB_DOC_ID,
                                          JOB_DOC_ID_LEN,
                                          AFR_OTA_DOCUMENT,
                                          AFR_OTA_DOCUMENT_LENGTH );

    TEST_ASSERT_TRUE( result );
}

void test_handleJobDoc_returnsTrue_whenMultiFileIOTOtaJob( void )
{
    expectPopulateJobDocWithFileIndex( MULTI_FILE_OTA_DOCUMENT,
                                       MULTI_FILE_OTA_DOCUMENT_LENGTH,
                                       0 );
    expectPopulateJobDocWithFileIndex( MULTI_FILE_OTA_DOCUMENT,
                                       MULTI_FILE_OTA_DOCUMENT_LENGTH,
                                       1 );
    expectPopulateJobDocWithFileIndex( MULTI_FILE_OTA_DOCUMENT,
                                       MULTI_FILE_OTA_DOCUMENT_LENGTH,
                                       2 );

    bool result = otaParser_handleJobDoc( &verifyCallbackValues,
                                          JOB_DOC_ID,
                                          JOB_DOC_ID_LEN,
                                          MULTI_FILE_OTA_DOCUMENT,
                                          MULTI_FILE_OTA_DOCUMENT_LENGTH );

    TEST_ASSERT_TRUE( result );
}

void test_handleJobDoc_returnsTrue_whenTooManyFilesInIOTOtaJob( void )
{
    expectPopulateJobDocWithFileIndex( TOO_MANY_FILES_OTA_DOCUMENT,
                                       TOO_MANY_FILES_OTA_DOCUMENT_LENGTH,
                                       0 );
    expectPopulateJobDocWithFileIndex( TOO_MANY_FILES_OTA_DOCUMENT,
                                       TOO_MANY_FILES_OTA_DOCUMENT_LENGTH,
                                       1 );
    expectPopulateJobDocWithFileIndex( TOO_MANY_FILES_OTA_DOCUMENT,
                                       TOO_MANY_FILES_OTA_DOCUMENT_LENGTH,
                                       2 );
    expectPopulateJobDocWithFileIndex( TOO_MANY_FILES_OTA_DOCUMENT,
                                       TOO_MANY_FILES_OTA_DOCUMENT_LENGTH,
                                       3 );
    expectPopulateJobDocWithFileIndex( TOO_MANY_FILES_OTA_DOCUMENT,
                                       TOO_MANY_FILES_OTA_DOCUMENT_LENGTH,
                                       4 );
    expectPopulateJobDocWithFileIndex( TOO_MANY_FILES_OTA_DOCUMENT,
                                       TOO_MANY_FILES_OTA_DOCUMENT_LENGTH,
                                       5 );
    expectPopulateJobDocWithFileIndex( TOO_MANY_FILES_OTA_DOCUMENT,
                                       TOO_MANY_FILES_OTA_DOCUMENT_LENGTH,
                                       6 );
    expectPopulateJobDocWithFileIndex( TOO_MANY_FILES_OTA_DOCUMENT,
                                       TOO_MANY_FILES_OTA_DOCUMENT_LENGTH,
                                       7 );
    expectPopulateJobDocWithFileIndex( TOO_MANY_FILES_OTA_DOCUMENT,
                                       TOO_MANY_FILES_OTA_DOCUMENT_LENGTH,
                                       8 );
    expectPopulateJobDocWithFileIndex( TOO_MANY_FILES_OTA_DOCUMENT,
                                       TOO_MANY_FILES_OTA_DOCUMENT_LENGTH,
                                       9 );

    bool result = otaParser_handleJobDoc( &verifyCallbackValues,
                                          JOB_DOC_ID,
                                          JOB_DOC_ID_LEN,
                                          TOO_MANY_FILES_OTA_DOCUMENT,
                                          TOO_MANY_FILES_OTA_DOCUMENT_LENGTH );

    TEST_ASSERT_TRUE( result );
}

void test_handleJobDoc_returnsFalse_whenParsingFails( void )
{
    populateJobDocFields_ExpectAndReturn( AFR_OTA_DOCUMENT,
                                          AFR_OTA_DOCUMENT_LENGTH,
                                          0,
                                          NULL,
                                          false );
    populateJobDocFields_IgnoreArg_result();

    bool result = otaParser_handleJobDoc( &verifyCallbackValues,
                                          JOB_DOC_ID,
                                          JOB_DOC_ID_LEN,
                                          AFR_OTA_DOCUMENT,
                                          AFR_OTA_DOCUMENT_LENGTH );

    TEST_ASSERT_FALSE( result );
}

void test_handleJobDoc_returnsFalse_whenMultiFileParsingFails( void )
{
    expectPopulateJobDocWithFileIndex( MULTI_FILE_OTA_DOCUMENT,
                                       MULTI_FILE_OTA_DOCUMENT_LENGTH,
                                       0 );
    populateJobDocFields_ExpectAndReturn( MULTI_FILE_OTA_DOCUMENT,
                                          MULTI_FILE_OTA_DOCUMENT_LENGTH,
                                          1,
                                          NULL,
                                          false );
    populateJobDocFields_IgnoreArg_result();

    bool result = otaParser_handleJobDoc( &verifyCallbackValues,
                                          JOB_DOC_ID,
                                          JOB_DOC_ID_LEN,
                                          MULTI_FILE_OTA_DOCUMENT,
                                          MULTI_FILE_OTA_DOCUMENT_LENGTH );

    TEST_ASSERT_FALSE( result );
}

void test_handleJobDoc_returnsFalse_whenCustomJob( void )
{
    bool result = otaParser_handleJobDoc( &verifyCallbackValues,
                                          JOB_DOC_ID,
                                          JOB_DOC_ID_LEN,
                                          CUSTOM_DOCUMENT,
                                          CUSTOM_DOCUMENT_LENGTH );

    TEST_ASSERT_FALSE( result );
}

void test_handleJobDoc_returnsFalse_givenNullJobDocument( void )
{
    bool result = otaParser_handleJobDoc( &verifyCallbackValues,
                                          JOB_DOC_ID,
                                          JOB_DOC_ID_LEN,
                                          NULL,
                                          CUSTOM_DOCUMENT_LENGTH );

    TEST_ASSERT_FALSE( result );
}

void test_handleJobDoc_returnsFalse_givenZeroDocumentLength( void )
{
    bool result = otaParser_handleJobDoc( &verifyCallbackValues,
                                          JOB_DOC_ID,
                                          JOB_DOC_ID_LEN,
                                          AFR_OTA_DOCUMENT,
                                          0U );

    TEST_ASSERT_FALSE( result );
}