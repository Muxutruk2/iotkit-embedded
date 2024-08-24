/*                                                                                                                   
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited                                                             
 */                                                                                                                  




#if !defined(MBEDTLS_CONFIG_FILE                                                                                    )
#include "mbedtls/config.h"                                                                                          
#else                                                                                                                
#include MBEDTLS_CONFIG_FILE                                                                                         
#endif                                                                                                               

#if defined(MBEDTLS_SSL_TLS_C                                                                                       )

#if defined(MBEDTLS_PLATFORM_C                                                                                      )
#include "mbedtls/platform.h"                                                                                        
#else                                                                                                                
#include <stdlib.h>                                                                                                  
#endif                                                                                                               

#include "mbedtls/ssl_ciphersuites.h"                                                                                
#include "mbedtls/ssl.h"                                                                                             

#include <string.h>                                                                                                  

/*                                                                                                                   
 * Ordered from most preferred to least preferred in terms of security.                                              
 *                                                                                                                   
 * Current rule (except rc4, weak and null which come last):                                                         
 * 1. By key exchange:                                                                                               
 *    Forward-secure non-PSK > forward-secure PSK > ECJPAKE > other non-PSK > other PSK                              
 * 2. By key length and cipher:                                                                                      
 *    AES-256 > Camellia-256 > AES-128 > Camellia-128 > 3DES                                                         
 * 3. By cipher mode when relevant GCM > CCM > CBC > CCM_8                                                           
 * 4. By hash function used when relevant                                                                            
 * 5. By key exchange/auth again: EC > non-EC                                                                        
 */                                                                                                                  
static const int ciphersuite_preference[] =                                                                          
{
#if defined(MBEDTLS_SSL_CIPHERSUITES                                                                                )
    MBEDTLS_SSL_CIPHERSUITES,                                                                                        
#else                                                                                                                
    /* All AES-256 ephemeral suites */                                                                               
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384,                                                                 
    MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384,                                                                   
    MBEDTLS_TLS_DHE_RSA_WITH_AES_256_GCM_SHA384,                                                                     
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_CCM,                                                                        
    MBEDTLS_TLS_DHE_RSA_WITH_AES_256_CCM,                                                                            
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384,                                                                 
    MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384,                                                                   
    MBEDTLS_TLS_DHE_RSA_WITH_AES_256_CBC_SHA256,                                                                     
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA,                                                                    
    MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA,                                                                      
    MBEDTLS_TLS_DHE_RSA_WITH_AES_256_CBC_SHA,                                                                        
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_CCM_8,                                                                      
    MBEDTLS_TLS_DHE_RSA_WITH_AES_256_CCM_8,                                                                          

    /* All CAMELLIA-256 ephemeral suites */                                                                          
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_CAMELLIA_256_GCM_SHA384,                                                            
    MBEDTLS_TLS_ECDHE_RSA_WITH_CAMELLIA_256_GCM_SHA384,                                                              
    MBEDTLS_TLS_DHE_RSA_WITH_CAMELLIA_256_GCM_SHA384,                                                                
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_CAMELLIA_256_CBC_SHA384,                                                            
    MBEDTLS_TLS_ECDHE_RSA_WITH_CAMELLIA_256_CBC_SHA384,                                                              
    MBEDTLS_TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA256,                                                                
    MBEDTLS_TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA,                                                                   

    /* All AES-128 ephemeral suites */                                                                               
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,                                                                 
    MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,                                                                   
    MBEDTLS_TLS_DHE_RSA_WITH_AES_128_GCM_SHA256,                                                                     
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CCM,                                                                        
    MBEDTLS_TLS_DHE_RSA_WITH_AES_128_CCM,                                                                            
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256,                                                                 
    MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256,                                                                   
    MBEDTLS_TLS_DHE_RSA_WITH_AES_128_CBC_SHA256,                                                                     
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA,                                                                    
    MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA,                                                                      
    MBEDTLS_TLS_DHE_RSA_WITH_AES_128_CBC_SHA,                                                                        
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8,                                                                      
    MBEDTLS_TLS_DHE_RSA_WITH_AES_128_CCM_8,                                                                          

    /* All CAMELLIA-128 ephemeral suites */                                                                          
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_CAMELLIA_128_GCM_SHA256,                                                            
    MBEDTLS_TLS_ECDHE_RSA_WITH_CAMELLIA_128_GCM_SHA256,                                                              
    MBEDTLS_TLS_DHE_RSA_WITH_CAMELLIA_128_GCM_SHA256,                                                                
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_CAMELLIA_128_CBC_SHA256,                                                            
    MBEDTLS_TLS_ECDHE_RSA_WITH_CAMELLIA_128_CBC_SHA256,                                                              
    MBEDTLS_TLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA256,                                                                
    MBEDTLS_TLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA,                                                                   

    /* All remaining >= 128-bit ephemeral suites */                                                                  
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA,                                                                   
    MBEDTLS_TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA,                                                                     
    MBEDTLS_TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA,                                                                       

    /* The PSK ephemeral suites */                                                                                   
    MBEDTLS_TLS_DHE_PSK_WITH_AES_256_GCM_SHA384,                                                                     
    MBEDTLS_TLS_DHE_PSK_WITH_AES_256_CCM,                                                                            
    MBEDTLS_TLS_ECDHE_PSK_WITH_AES_256_CBC_SHA384,                                                                   
    MBEDTLS_TLS_DHE_PSK_WITH_AES_256_CBC_SHA384,                                                                     
    MBEDTLS_TLS_ECDHE_PSK_WITH_AES_256_CBC_SHA,                                                                      
    MBEDTLS_TLS_DHE_PSK_WITH_AES_256_CBC_SHA,                                                                        
    MBEDTLS_TLS_DHE_PSK_WITH_CAMELLIA_256_GCM_SHA384,                                                                
    MBEDTLS_TLS_ECDHE_PSK_WITH_CAMELLIA_256_CBC_SHA384,                                                              
    MBEDTLS_TLS_DHE_PSK_WITH_CAMELLIA_256_CBC_SHA384,                                                                
    MBEDTLS_TLS_DHE_PSK_WITH_AES_256_CCM_8,                                                                          

    MBEDTLS_TLS_DHE_PSK_WITH_AES_128_GCM_SHA256,                                                                     
    MBEDTLS_TLS_DHE_PSK_WITH_AES_128_CCM,                                                                            
    MBEDTLS_TLS_ECDHE_PSK_WITH_AES_128_CBC_SHA256,                                                                   
    MBEDTLS_TLS_DHE_PSK_WITH_AES_128_CBC_SHA256,                                                                     
    MBEDTLS_TLS_ECDHE_PSK_WITH_AES_128_CBC_SHA,                                                                      
    MBEDTLS_TLS_DHE_PSK_WITH_AES_128_CBC_SHA,                                                                        
    MBEDTLS_TLS_DHE_PSK_WITH_CAMELLIA_128_GCM_SHA256,                                                                
    MBEDTLS_TLS_DHE_PSK_WITH_CAMELLIA_128_CBC_SHA256,                                                                
    MBEDTLS_TLS_ECDHE_PSK_WITH_CAMELLIA_128_CBC_SHA256,                                                              
    MBEDTLS_TLS_DHE_PSK_WITH_AES_128_CCM_8,                                                                          

    MBEDTLS_TLS_ECDHE_PSK_WITH_3DES_EDE_CBC_SHA,                                                                     
    MBEDTLS_TLS_DHE_PSK_WITH_3DES_EDE_CBC_SHA,                                                                       

    /* The ECJPAKE suite */                                                                                          
    MBEDTLS_TLS_ECJPAKE_WITH_AES_128_CCM_8,                                                                          

    /* All AES-256 suites */                                                                                         
    MBEDTLS_TLS_RSA_WITH_AES_256_GCM_SHA384,                                                                         
    MBEDTLS_TLS_RSA_WITH_AES_256_CCM,                                                                                
    MBEDTLS_TLS_RSA_WITH_AES_256_CBC_SHA256,                                                                         
    MBEDTLS_TLS_RSA_WITH_AES_256_CBC_SHA,                                                                            
    MBEDTLS_TLS_ECDH_RSA_WITH_AES_256_GCM_SHA384,                                                                    
    MBEDTLS_TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384,                                                                    
    MBEDTLS_TLS_ECDH_RSA_WITH_AES_256_CBC_SHA,                                                                       
    MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_256_GCM_SHA384,                                                                  
    MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384,                                                                  
    MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA,                                                                     
    MBEDTLS_TLS_RSA_WITH_AES_256_CCM_8,                                                                              

    /* All CAMELLIA-256 suites */                                                                                    
    MBEDTLS_TLS_RSA_WITH_CAMELLIA_256_GCM_SHA384,                                                                    
    MBEDTLS_TLS_RSA_WITH_CAMELLIA_256_CBC_SHA256,                                                                    
    MBEDTLS_TLS_RSA_WITH_CAMELLIA_256_CBC_SHA,                                                                       
    MBEDTLS_TLS_ECDH_RSA_WITH_CAMELLIA_256_GCM_SHA384,                                                               
    MBEDTLS_TLS_ECDH_RSA_WITH_CAMELLIA_256_CBC_SHA384,                                                               
    MBEDTLS_TLS_ECDH_ECDSA_WITH_CAMELLIA_256_GCM_SHA384,                                                             
    MBEDTLS_TLS_ECDH_ECDSA_WITH_CAMELLIA_256_CBC_SHA384,                                                             

    /* All AES-128 suites */                                                                                         
    MBEDTLS_TLS_RSA_WITH_AES_128_GCM_SHA256,                                                                         
    MBEDTLS_TLS_RSA_WITH_AES_128_CCM,                                                                                
    MBEDTLS_TLS_RSA_WITH_AES_128_CBC_SHA256,                                                                         
    MBEDTLS_TLS_RSA_WITH_AES_128_CBC_SHA,                                                                            
    MBEDTLS_TLS_ECDH_RSA_WITH_AES_128_GCM_SHA256,                                                                    
    MBEDTLS_TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256,                                                                    
    MBEDTLS_TLS_ECDH_RSA_WITH_AES_128_CBC_SHA,                                                                       
    MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256,                                                                  
    MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256,                                                                  
    MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA,                                                                     
    MBEDTLS_TLS_RSA_WITH_AES_128_CCM_8,                                                                              

    /* All CAMELLIA-128 suites */                                                                                    
    MBEDTLS_TLS_RSA_WITH_CAMELLIA_128_GCM_SHA256,                                                                    
    MBEDTLS_TLS_RSA_WITH_CAMELLIA_128_CBC_SHA256,                                                                    
    MBEDTLS_TLS_RSA_WITH_CAMELLIA_128_CBC_SHA,                                                                       
    MBEDTLS_TLS_ECDH_RSA_WITH_CAMELLIA_128_GCM_SHA256,                                                               
    MBEDTLS_TLS_ECDH_RSA_WITH_CAMELLIA_128_CBC_SHA256,                                                               
    MBEDTLS_TLS_ECDH_ECDSA_WITH_CAMELLIA_128_GCM_SHA256,                                                             
    MBEDTLS_TLS_ECDH_ECDSA_WITH_CAMELLIA_128_CBC_SHA256,                                                             

    /* All remaining >= 128-bit suites */                                                                            
    MBEDTLS_TLS_RSA_WITH_3DES_EDE_CBC_SHA,                                                                           
    MBEDTLS_TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA,                                                                      
    MBEDTLS_TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA,                                                                    

    /* The RSA PSK suites */                                                                                         
    MBEDTLS_TLS_RSA_PSK_WITH_AES_256_GCM_SHA384,                                                                     
    MBEDTLS_TLS_RSA_PSK_WITH_AES_256_CBC_SHA384,                                                                     
    MBEDTLS_TLS_RSA_PSK_WITH_AES_256_CBC_SHA,                                                                        
    MBEDTLS_TLS_RSA_PSK_WITH_CAMELLIA_256_GCM_SHA384,                                                                
    MBEDTLS_TLS_RSA_PSK_WITH_CAMELLIA_256_CBC_SHA384,                                                                

    MBEDTLS_TLS_RSA_PSK_WITH_AES_128_GCM_SHA256,                                                                     
    MBEDTLS_TLS_RSA_PSK_WITH_AES_128_CBC_SHA256,                                                                     
    MBEDTLS_TLS_RSA_PSK_WITH_AES_128_CBC_SHA,                                                                        
    MBEDTLS_TLS_RSA_PSK_WITH_CAMELLIA_128_GCM_SHA256,                                                                
    MBEDTLS_TLS_RSA_PSK_WITH_CAMELLIA_128_CBC_SHA256,                                                                

    MBEDTLS_TLS_RSA_PSK_WITH_3DES_EDE_CBC_SHA,                                                                       

    /* The PSK suites */                                                                                             
    MBEDTLS_TLS_PSK_WITH_AES_256_GCM_SHA384,                                                                         
    MBEDTLS_TLS_PSK_WITH_AES_256_CCM,                                                                                
    MBEDTLS_TLS_PSK_WITH_AES_256_CBC_SHA384,                                                                         
    MBEDTLS_TLS_PSK_WITH_AES_256_CBC_SHA,                                                                            
    MBEDTLS_TLS_PSK_WITH_CAMELLIA_256_GCM_SHA384,                                                                    
    MBEDTLS_TLS_PSK_WITH_CAMELLIA_256_CBC_SHA384,                                                                    
    MBEDTLS_TLS_PSK_WITH_AES_256_CCM_8,                                                                              

    MBEDTLS_TLS_PSK_WITH_AES_128_GCM_SHA256,                                                                         
    MBEDTLS_TLS_PSK_WITH_AES_128_CCM,                                                                                
    MBEDTLS_TLS_PSK_WITH_AES_128_CBC_SHA256,                                                                         
    MBEDTLS_TLS_PSK_WITH_AES_128_CBC_SHA,                                                                            
    MBEDTLS_TLS_PSK_WITH_CAMELLIA_128_GCM_SHA256,                                                                    
    MBEDTLS_TLS_PSK_WITH_CAMELLIA_128_CBC_SHA256,                                                                    
    MBEDTLS_TLS_PSK_WITH_AES_128_CCM_8,                                                                              

    MBEDTLS_TLS_PSK_WITH_3DES_EDE_CBC_SHA,                                                                           

    /* RC4 suites */                                                                                                 
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_RC4_128_SHA,                                                                        
    MBEDTLS_TLS_ECDHE_RSA_WITH_RC4_128_SHA,                                                                          
    MBEDTLS_TLS_ECDHE_PSK_WITH_RC4_128_SHA,                                                                          
    MBEDTLS_TLS_DHE_PSK_WITH_RC4_128_SHA,                                                                            
    MBEDTLS_TLS_RSA_WITH_RC4_128_SHA,                                                                                
    MBEDTLS_TLS_RSA_WITH_RC4_128_MD5,                                                                                
    MBEDTLS_TLS_ECDH_RSA_WITH_RC4_128_SHA,                                                                           
    MBEDTLS_TLS_ECDH_ECDSA_WITH_RC4_128_SHA,                                                                         
    MBEDTLS_TLS_RSA_PSK_WITH_RC4_128_SHA,                                                                            
    MBEDTLS_TLS_PSK_WITH_RC4_128_SHA,                                                                                

    /* Weak suites */                                                                                                
    MBEDTLS_TLS_DHE_RSA_WITH_DES_CBC_SHA,                                                                            
    MBEDTLS_TLS_RSA_WITH_DES_CBC_SHA,                                                                                

    /* NULL suites */                                                                                                
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_NULL_SHA,                                                                           
    MBEDTLS_TLS_ECDHE_RSA_WITH_NULL_SHA,                                                                             
    MBEDTLS_TLS_ECDHE_PSK_WITH_NULL_SHA384,                                                                          
    MBEDTLS_TLS_ECDHE_PSK_WITH_NULL_SHA256,                                                                          
    MBEDTLS_TLS_ECDHE_PSK_WITH_NULL_SHA,                                                                             
    MBEDTLS_TLS_DHE_PSK_WITH_NULL_SHA384,                                                                            
    MBEDTLS_TLS_DHE_PSK_WITH_NULL_SHA256,                                                                            
    MBEDTLS_TLS_DHE_PSK_WITH_NULL_SHA,                                                                               

    MBEDTLS_TLS_RSA_WITH_NULL_SHA256,                                                                                
    MBEDTLS_TLS_RSA_WITH_NULL_SHA,                                                                                   
    MBEDTLS_TLS_RSA_WITH_NULL_MD5,                                                                                   
    MBEDTLS_TLS_ECDH_RSA_WITH_NULL_SHA,                                                                              
    MBEDTLS_TLS_ECDH_ECDSA_WITH_NULL_SHA,                                                                            
    MBEDTLS_TLS_RSA_PSK_WITH_NULL_SHA384,                                                                            
    MBEDTLS_TLS_RSA_PSK_WITH_NULL_SHA256,                                                                            
    MBEDTLS_TLS_RSA_PSK_WITH_NULL_SHA,                                                                               
    MBEDTLS_TLS_PSK_WITH_NULL_SHA384,                                                                                
    MBEDTLS_TLS_PSK_WITH_NULL_SHA256,                                                                                
    MBEDTLS_TLS_PSK_WITH_NULL_SHA,                                                                                   

#endif /* MBEDTLS_SSL_CIPHERSUITES */                                                                                
    0                                                                                                                
};

static const mbedtls_ssl_ciphersuite_t ciphersuite_definitions[] =                                                   
{
#if defined(MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED                                                                )
#if defined(MBEDTLS_AES_C                                                                                           )
#if defined(MBEDTLS_SHA1_C                                                                                          )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
    { MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA, "TLS-ECDHE-ECDSA-WITH-AES-128-CBC-SHA",                          
      MBEDTLS_CIPHER_AES_128_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA,                                 
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
    { MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA, "TLS-ECDHE-ECDSA-WITH-AES-256-CBC-SHA",                          
      MBEDTLS_CIPHER_AES_256_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA,                                 
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#endif /* MBEDTLS_SHA1_C */                                                                                          
#if defined(MBEDTLS_SHA256_C                                                                                        )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
    { MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256, "TLS-ECDHE-ECDSA-WITH-AES-128-CBC-SHA256",                    
      MBEDTLS_CIPHER_AES_128_CBC, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA,                               
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#if defined(MBEDTLS_GCM_C                                                                                           )
    { MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256, "TLS-ECDHE-ECDSA-WITH-AES-128-GCM-SHA256",                    
      MBEDTLS_CIPHER_AES_128_GCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA,                               
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_GCM_C */                                                                                           
#endif /* MBEDTLS_SHA256_C */                                                                                        
#if defined(MBEDTLS_SHA512_C                                                                                        )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
    { MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384, "TLS-ECDHE-ECDSA-WITH-AES-256-CBC-SHA384",                    
      MBEDTLS_CIPHER_AES_256_CBC, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA,                               
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#if defined(MBEDTLS_GCM_C                                                                                           )
    { MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384, "TLS-ECDHE-ECDSA-WITH-AES-256-GCM-SHA384",                    
      MBEDTLS_CIPHER_AES_256_GCM, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA,                               
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_GCM_C */                                                                                           
#endif /* MBEDTLS_SHA512_C */                                                                                        
#if defined(MBEDTLS_CCM_C                                                                                           )
    { MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_CCM, "TLS-ECDHE-ECDSA-WITH-AES-256-CCM",                                  
      MBEDTLS_CIPHER_AES_256_CCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA,                               
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
    { MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_CCM_8, "TLS-ECDHE-ECDSA-WITH-AES-256-CCM-8",                              
      MBEDTLS_CIPHER_AES_256_CCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA,                               
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_SHORT_TAG },                                                                               
    { MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CCM, "TLS-ECDHE-ECDSA-WITH-AES-128-CCM",                                  
      MBEDTLS_CIPHER_AES_128_CCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA,                               
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
    { MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8, "TLS-ECDHE-ECDSA-WITH-AES-128-CCM-8",                              
      MBEDTLS_CIPHER_AES_128_CCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA,                               
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_SHORT_TAG },                                                                               
#endif /* MBEDTLS_CCM_C */                                                                                           
#endif /* MBEDTLS_AES_C */                                                                                           

#if defined(MBEDTLS_CAMELLIA_C                                                                                      )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_ECDHE_ECDSA_WITH_CAMELLIA_128_CBC_SHA256, "TLS-ECDHE-ECDSA-WITH-CAMELLIA-128-CBC-SHA256",          
      MBEDTLS_CIPHER_CAMELLIA_128_CBC, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA,                          
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA256_C */                                                                                        
#if defined(MBEDTLS_SHA512_C                                                                                        )
    { MBEDTLS_TLS_ECDHE_ECDSA_WITH_CAMELLIA_256_CBC_SHA384, "TLS-ECDHE-ECDSA-WITH-CAMELLIA-256-CBC-SHA384",          
      MBEDTLS_CIPHER_CAMELLIA_256_CBC, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA,                          
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA512_C */                                                                                        
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 

#if defined(MBEDTLS_GCM_C                                                                                           )
#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_ECDHE_ECDSA_WITH_CAMELLIA_128_GCM_SHA256, "TLS-ECDHE-ECDSA-WITH-CAMELLIA-128-GCM-SHA256",          
      MBEDTLS_CIPHER_CAMELLIA_128_GCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA,                          
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA256_C */                                                                                        
#if defined(MBEDTLS_SHA512_C                                                                                        )
    { MBEDTLS_TLS_ECDHE_ECDSA_WITH_CAMELLIA_256_GCM_SHA384, "TLS-ECDHE-ECDSA-WITH-CAMELLIA-256-GCM-SHA384",          
      MBEDTLS_CIPHER_CAMELLIA_256_GCM, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA,                          
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA512_C */                                                                                        
#endif /* MBEDTLS_GCM_C */                                                                                           
#endif /* MBEDTLS_CAMELLIA_C */                                                                                      

#if defined(MBEDTLS_DES_C                                                                                           )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA, "TLS-ECDHE-ECDSA-WITH-3DES-EDE-CBC-SHA",                        
      MBEDTLS_CIPHER_DES_EDE3_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA,                                
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#endif /* MBEDTLS_DES_C */                                                                                           

#if defined(MBEDTLS_ARC4_C                                                                                          )
#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_ECDHE_ECDSA_WITH_RC4_128_SHA, "TLS-ECDHE-ECDSA-WITH-RC4-128-SHA",                                  
      MBEDTLS_CIPHER_ARC4_128, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA,                                    
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_NODTLS },                                                                                  
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_ARC4_C */                                                                                          

#if defined(MBEDTLS_CIPHER_NULL_CIPHER                                                                              )
#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_ECDHE_ECDSA_WITH_NULL_SHA, "TLS-ECDHE-ECDSA-WITH-NULL-SHA",                                        
      MBEDTLS_CIPHER_NULL, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA,                                        
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_WEAK },                                                                                    
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_CIPHER_NULL_CIPHER */                                                                              
#endif /* MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED */                                                                

#if defined(MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED                                                                  )
#if defined(MBEDTLS_AES_C                                                                                           )
#if defined(MBEDTLS_SHA1_C                                                                                          )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
    { MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA, "TLS-ECDHE-RSA-WITH-AES-128-CBC-SHA",                              
      MBEDTLS_CIPHER_AES_128_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_ECDHE_RSA,                                   
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
    { MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA, "TLS-ECDHE-RSA-WITH-AES-256-CBC-SHA",                              
      MBEDTLS_CIPHER_AES_256_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_ECDHE_RSA,                                   
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#endif /* MBEDTLS_SHA1_C */                                                                                          
#if defined(MBEDTLS_SHA256_C                                                                                        )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
    { MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256, "TLS-ECDHE-RSA-WITH-AES-128-CBC-SHA256",                        
      MBEDTLS_CIPHER_AES_128_CBC, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_ECDHE_RSA,                                 
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#if defined(MBEDTLS_GCM_C                                                                                           )
    { MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256, "TLS-ECDHE-RSA-WITH-AES-128-GCM-SHA256",                        
      MBEDTLS_CIPHER_AES_128_GCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_ECDHE_RSA,                                 
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_GCM_C */                                                                                           
#endif /* MBEDTLS_SHA256_C */                                                                                        
#if defined(MBEDTLS_SHA512_C                                                                                        )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
    { MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384, "TLS-ECDHE-RSA-WITH-AES-256-CBC-SHA384",                        
      MBEDTLS_CIPHER_AES_256_CBC, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_ECDHE_RSA,                                 
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#if defined(MBEDTLS_GCM_C                                                                                           )
    { MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384, "TLS-ECDHE-RSA-WITH-AES-256-GCM-SHA384",                        
      MBEDTLS_CIPHER_AES_256_GCM, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_ECDHE_RSA,                                 
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_GCM_C */                                                                                           
#endif /* MBEDTLS_SHA512_C */                                                                                        
#endif /* MBEDTLS_AES_C */                                                                                           

#if defined(MBEDTLS_CAMELLIA_C                                                                                      )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_ECDHE_RSA_WITH_CAMELLIA_128_CBC_SHA256, "TLS-ECDHE-RSA-WITH-CAMELLIA-128-CBC-SHA256",              
      MBEDTLS_CIPHER_CAMELLIA_128_CBC, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_ECDHE_RSA,                            
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA256_C */                                                                                        
#if defined(MBEDTLS_SHA512_C                                                                                        )
    { MBEDTLS_TLS_ECDHE_RSA_WITH_CAMELLIA_256_CBC_SHA384, "TLS-ECDHE-RSA-WITH-CAMELLIA-256-CBC-SHA384",              
      MBEDTLS_CIPHER_CAMELLIA_256_CBC, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_ECDHE_RSA,                            
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA512_C */                                                                                        
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 

#if defined(MBEDTLS_GCM_C                                                                                           )
#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_ECDHE_RSA_WITH_CAMELLIA_128_GCM_SHA256, "TLS-ECDHE-RSA-WITH-CAMELLIA-128-GCM-SHA256",              
      MBEDTLS_CIPHER_CAMELLIA_128_GCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_ECDHE_RSA,                            
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA256_C */                                                                                        
#if defined(MBEDTLS_SHA512_C                                                                                        )
    { MBEDTLS_TLS_ECDHE_RSA_WITH_CAMELLIA_256_GCM_SHA384, "TLS-ECDHE-RSA-WITH-CAMELLIA-256-GCM-SHA384",              
      MBEDTLS_CIPHER_CAMELLIA_256_GCM, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_ECDHE_RSA,                            
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA512_C */                                                                                        
#endif /* MBEDTLS_GCM_C */                                                                                           
#endif /* MBEDTLS_CAMELLIA_C */                                                                                      

#if defined(MBEDTLS_DES_C                                                                                           )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA, "TLS-ECDHE-RSA-WITH-3DES-EDE-CBC-SHA",                            
      MBEDTLS_CIPHER_DES_EDE3_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_ECDHE_RSA,                                  
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#endif /* MBEDTLS_DES_C */                                                                                           

#if defined(MBEDTLS_ARC4_C                                                                                          )
#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_ECDHE_RSA_WITH_RC4_128_SHA, "TLS-ECDHE-RSA-WITH-RC4-128-SHA",                                      
      MBEDTLS_CIPHER_ARC4_128, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_ECDHE_RSA,                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_NODTLS },                                                                                  
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_ARC4_C */                                                                                          

#if defined(MBEDTLS_CIPHER_NULL_CIPHER                                                                              )
#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_ECDHE_RSA_WITH_NULL_SHA, "TLS-ECDHE-RSA-WITH-NULL-SHA",                                            
      MBEDTLS_CIPHER_NULL, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_ECDHE_RSA,                                          
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_WEAK },                                                                                    
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_CIPHER_NULL_CIPHER */                                                                              
#endif /* MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED */                                                                  

#if defined(MBEDTLS_KEY_EXCHANGE_DHE_RSA_ENABLED                                                                    )
#if defined(MBEDTLS_AES_C                                                                                           )
#if defined(MBEDTLS_SHA512_C) && defined(MBEDTLS_GCM_C                                                              )
    { MBEDTLS_TLS_DHE_RSA_WITH_AES_256_GCM_SHA384, "TLS-DHE-RSA-WITH-AES-256-GCM-SHA384",                            
      MBEDTLS_CIPHER_AES_256_GCM, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_DHE_RSA,                                   
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA512_C && MBEDTLS_GCM_C */                                                                       

#if defined(MBEDTLS_SHA256_C                                                                                        )
#if defined(MBEDTLS_GCM_C                                                                                           )
    { MBEDTLS_TLS_DHE_RSA_WITH_AES_128_GCM_SHA256, "TLS-DHE-RSA-WITH-AES-128-GCM-SHA256",                            
      MBEDTLS_CIPHER_AES_128_GCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_DHE_RSA,                                   
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_GCM_C */                                                                                           

#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
    { MBEDTLS_TLS_DHE_RSA_WITH_AES_128_CBC_SHA256, "TLS-DHE-RSA-WITH-AES-128-CBC-SHA256",                            
      MBEDTLS_CIPHER_AES_128_CBC, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_DHE_RSA,                                   
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           

    { MBEDTLS_TLS_DHE_RSA_WITH_AES_256_CBC_SHA256, "TLS-DHE-RSA-WITH-AES-256-CBC-SHA256",                            
      MBEDTLS_CIPHER_AES_256_CBC, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_DHE_RSA,                                   
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#endif /* MBEDTLS_SHA256_C */                                                                                        

#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_DHE_RSA_WITH_AES_128_CBC_SHA, "TLS-DHE-RSA-WITH-AES-128-CBC-SHA",                                  
      MBEDTLS_CIPHER_AES_128_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_DHE_RSA,                                     
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_0,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           

    { MBEDTLS_TLS_DHE_RSA_WITH_AES_256_CBC_SHA, "TLS-DHE-RSA-WITH-AES-256-CBC-SHA",                                  
      MBEDTLS_CIPHER_AES_256_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_DHE_RSA,                                     
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_0,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#if defined(MBEDTLS_CCM_C                                                                                           )
    { MBEDTLS_TLS_DHE_RSA_WITH_AES_256_CCM, "TLS-DHE-RSA-WITH-AES-256-CCM",                                          
      MBEDTLS_CIPHER_AES_256_CCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_DHE_RSA,                                   
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
    { MBEDTLS_TLS_DHE_RSA_WITH_AES_256_CCM_8, "TLS-DHE-RSA-WITH-AES-256-CCM-8",                                      
      MBEDTLS_CIPHER_AES_256_CCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_DHE_RSA,                                   
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_SHORT_TAG },                                                                               
    { MBEDTLS_TLS_DHE_RSA_WITH_AES_128_CCM, "TLS-DHE-RSA-WITH-AES-128-CCM",                                          
      MBEDTLS_CIPHER_AES_128_CCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_DHE_RSA,                                   
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
    { MBEDTLS_TLS_DHE_RSA_WITH_AES_128_CCM_8, "TLS-DHE-RSA-WITH-AES-128-CCM-8",                                      
      MBEDTLS_CIPHER_AES_128_CCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_DHE_RSA,                                   
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_SHORT_TAG },                                                                               
#endif /* MBEDTLS_CCM_C */                                                                                           
#endif /* MBEDTLS_AES_C */                                                                                           

#if defined(MBEDTLS_CAMELLIA_C                                                                                      )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA256, "TLS-DHE-RSA-WITH-CAMELLIA-128-CBC-SHA256",                  
      MBEDTLS_CIPHER_CAMELLIA_128_CBC, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_DHE_RSA,                              
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           

    { MBEDTLS_TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA256, "TLS-DHE-RSA-WITH-CAMELLIA-256-CBC-SHA256",                  
      MBEDTLS_CIPHER_CAMELLIA_256_CBC, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_DHE_RSA,                              
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA256_C */                                                                                        

#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA, "TLS-DHE-RSA-WITH-CAMELLIA-128-CBC-SHA",                        
      MBEDTLS_CIPHER_CAMELLIA_128_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_DHE_RSA,                                
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_0,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           

    { MBEDTLS_TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA, "TLS-DHE-RSA-WITH-CAMELLIA-256-CBC-SHA",                        
      MBEDTLS_CIPHER_CAMELLIA_256_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_DHE_RSA,                                
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_0,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#if defined(MBEDTLS_GCM_C                                                                                           )
#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_DHE_RSA_WITH_CAMELLIA_128_GCM_SHA256, "TLS-DHE-RSA-WITH-CAMELLIA-128-GCM-SHA256",                  
      MBEDTLS_CIPHER_CAMELLIA_128_GCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_DHE_RSA,                              
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA256_C */                                                                                        

#if defined(MBEDTLS_SHA512_C                                                                                        )
    { MBEDTLS_TLS_DHE_RSA_WITH_CAMELLIA_256_GCM_SHA384, "TLS-DHE-RSA-WITH-CAMELLIA-256-GCM-SHA384",                  
      MBEDTLS_CIPHER_CAMELLIA_256_GCM, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_DHE_RSA,                              
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA512_C */                                                                                        
#endif /* MBEDTLS_GCM_C */                                                                                           
#endif /* MBEDTLS_CAMELLIA_C */                                                                                      

#if defined(MBEDTLS_DES_C                                                                                           )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA, "TLS-DHE-RSA-WITH-3DES-EDE-CBC-SHA",                                
      MBEDTLS_CIPHER_DES_EDE3_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_DHE_RSA,                                    
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_0,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#endif /* MBEDTLS_DES_C */                                                                                           
#endif /* MBEDTLS_KEY_EXCHANGE_DHE_RSA_ENABLED */                                                                    

#if defined(MBEDTLS_KEY_EXCHANGE_RSA_ENABLED                                                                        )
#if defined(MBEDTLS_AES_C                                                                                           )
#if defined(MBEDTLS_SHA512_C) && defined(MBEDTLS_GCM_C                                                              )
    { MBEDTLS_TLS_RSA_WITH_AES_256_GCM_SHA384, "TLS-RSA-WITH-AES-256-GCM-SHA384",                                    
      MBEDTLS_CIPHER_AES_256_GCM, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_RSA,                                       
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA512_C && MBEDTLS_GCM_C */                                                                       

#if defined(MBEDTLS_SHA256_C                                                                                        )
#if defined(MBEDTLS_GCM_C                                                                                           )
    { MBEDTLS_TLS_RSA_WITH_AES_128_GCM_SHA256, "TLS-RSA-WITH-AES-128-GCM-SHA256",                                    
      MBEDTLS_CIPHER_AES_128_GCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_RSA,                                       
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_GCM_C */                                                                                           

#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
    { MBEDTLS_TLS_RSA_WITH_AES_128_CBC_SHA256, "TLS-RSA-WITH-AES-128-CBC-SHA256",                                    
      MBEDTLS_CIPHER_AES_128_CBC, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_RSA,                                       
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           

    { MBEDTLS_TLS_RSA_WITH_AES_256_CBC_SHA256, "TLS-RSA-WITH-AES-256-CBC-SHA256",                                    
      MBEDTLS_CIPHER_AES_256_CBC, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_RSA,                                       
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#endif /* MBEDTLS_SHA256_C */                                                                                        

#if defined(MBEDTLS_SHA1_C                                                                                          )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
    { MBEDTLS_TLS_RSA_WITH_AES_128_CBC_SHA, "TLS-RSA-WITH-AES-128-CBC-SHA",                                          
      MBEDTLS_CIPHER_AES_128_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_RSA,                                         
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_0,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           

    { MBEDTLS_TLS_RSA_WITH_AES_256_CBC_SHA, "TLS-RSA-WITH-AES-256-CBC-SHA",                                          
      MBEDTLS_CIPHER_AES_256_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_RSA,                                         
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_0,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#endif /* MBEDTLS_SHA1_C */                                                                                          
#if defined(MBEDTLS_CCM_C                                                                                           )
    { MBEDTLS_TLS_RSA_WITH_AES_256_CCM, "TLS-RSA-WITH-AES-256-CCM",                                                  
      MBEDTLS_CIPHER_AES_256_CCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_RSA,                                       
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
    { MBEDTLS_TLS_RSA_WITH_AES_256_CCM_8, "TLS-RSA-WITH-AES-256-CCM-8",                                              
      MBEDTLS_CIPHER_AES_256_CCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_RSA,                                       
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_SHORT_TAG },                                                                               
    { MBEDTLS_TLS_RSA_WITH_AES_128_CCM, "TLS-RSA-WITH-AES-128-CCM",                                                  
      MBEDTLS_CIPHER_AES_128_CCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_RSA,                                       
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
    { MBEDTLS_TLS_RSA_WITH_AES_128_CCM_8, "TLS-RSA-WITH-AES-128-CCM-8",                                              
      MBEDTLS_CIPHER_AES_128_CCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_RSA,                                       
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_SHORT_TAG },                                                                               
#endif /* MBEDTLS_CCM_C */                                                                                           
#endif /* MBEDTLS_AES_C */                                                                                           

#if defined(MBEDTLS_CAMELLIA_C                                                                                      )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_RSA_WITH_CAMELLIA_128_CBC_SHA256, "TLS-RSA-WITH-CAMELLIA-128-CBC-SHA256",                          
      MBEDTLS_CIPHER_CAMELLIA_128_CBC, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_RSA,                                  
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           

    { MBEDTLS_TLS_RSA_WITH_CAMELLIA_256_CBC_SHA256, "TLS-RSA-WITH-CAMELLIA-256-CBC-SHA256",                          
      MBEDTLS_CIPHER_CAMELLIA_256_CBC, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_RSA,                                  
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA256_C */                                                                                        

#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_RSA_WITH_CAMELLIA_128_CBC_SHA, "TLS-RSA-WITH-CAMELLIA-128-CBC-SHA",                                
      MBEDTLS_CIPHER_CAMELLIA_128_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_RSA,                                    
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_0,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           

    { MBEDTLS_TLS_RSA_WITH_CAMELLIA_256_CBC_SHA, "TLS-RSA-WITH-CAMELLIA-256-CBC-SHA",                                
      MBEDTLS_CIPHER_CAMELLIA_256_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_RSA,                                    
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_0,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 

#if defined(MBEDTLS_GCM_C                                                                                           )
#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_RSA_WITH_CAMELLIA_128_GCM_SHA256, "TLS-RSA-WITH-CAMELLIA-128-GCM-SHA256",                          
      MBEDTLS_CIPHER_CAMELLIA_128_GCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_RSA,                                  
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA256_C */                                                                                        

#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_RSA_WITH_CAMELLIA_256_GCM_SHA384, "TLS-RSA-WITH-CAMELLIA-256-GCM-SHA384",                          
      MBEDTLS_CIPHER_CAMELLIA_256_GCM, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_RSA,                                  
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_GCM_C */                                                                                           
#endif /* MBEDTLS_CAMELLIA_C */                                                                                      

#if defined(MBEDTLS_DES_C                                                                                           )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_RSA_WITH_3DES_EDE_CBC_SHA, "TLS-RSA-WITH-3DES-EDE-CBC-SHA",                                        
      MBEDTLS_CIPHER_DES_EDE3_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_RSA,                                        
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_0,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#endif /* MBEDTLS_DES_C */                                                                                           

#if defined(MBEDTLS_ARC4_C                                                                                          )
#if defined(MBEDTLS_MD5_C                                                                                           )
    { MBEDTLS_TLS_RSA_WITH_RC4_128_MD5, "TLS-RSA-WITH-RC4-128-MD5",                                                  
      MBEDTLS_CIPHER_ARC4_128, MBEDTLS_MD_MD5, MBEDTLS_KEY_EXCHANGE_RSA,                                             
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_0,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_NODTLS },                                                                                  
#endif                                                                                                               

#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_RSA_WITH_RC4_128_SHA, "TLS-RSA-WITH-RC4-128-SHA",                                                  
      MBEDTLS_CIPHER_ARC4_128, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_RSA,                                            
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_0,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_NODTLS },                                                                                  
#endif                                                                                                               
#endif /* MBEDTLS_ARC4_C */                                                                                          
#endif /* MBEDTLS_KEY_EXCHANGE_RSA_ENABLED */                                                                        

#if defined(MBEDTLS_KEY_EXCHANGE_ECDH_RSA_ENABLED                                                                   )
#if defined(MBEDTLS_AES_C                                                                                           )
#if defined(MBEDTLS_SHA1_C                                                                                          )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
    { MBEDTLS_TLS_ECDH_RSA_WITH_AES_128_CBC_SHA, "TLS-ECDH-RSA-WITH-AES-128-CBC-SHA",                                
      MBEDTLS_CIPHER_AES_128_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_ECDH_RSA,                                    
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
    { MBEDTLS_TLS_ECDH_RSA_WITH_AES_256_CBC_SHA, "TLS-ECDH-RSA-WITH-AES-256-CBC-SHA",                                
      MBEDTLS_CIPHER_AES_256_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_ECDH_RSA,                                    
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#endif /* MBEDTLS_SHA1_C */                                                                                          
#if defined(MBEDTLS_SHA256_C                                                                                        )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
    { MBEDTLS_TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256, "TLS-ECDH-RSA-WITH-AES-128-CBC-SHA256",                          
      MBEDTLS_CIPHER_AES_128_CBC, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_ECDH_RSA,                                  
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#if defined(MBEDTLS_GCM_C                                                                                           )
    { MBEDTLS_TLS_ECDH_RSA_WITH_AES_128_GCM_SHA256, "TLS-ECDH-RSA-WITH-AES-128-GCM-SHA256",                          
      MBEDTLS_CIPHER_AES_128_GCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_ECDH_RSA,                                  
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_GCM_C */                                                                                           
#endif /* MBEDTLS_SHA256_C */                                                                                        
#if defined(MBEDTLS_SHA512_C                                                                                        )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
    { MBEDTLS_TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384, "TLS-ECDH-RSA-WITH-AES-256-CBC-SHA384",                          
      MBEDTLS_CIPHER_AES_256_CBC, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_ECDH_RSA,                                  
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#if defined(MBEDTLS_GCM_C                                                                                           )
    { MBEDTLS_TLS_ECDH_RSA_WITH_AES_256_GCM_SHA384, "TLS-ECDH-RSA-WITH-AES-256-GCM-SHA384",                          
      MBEDTLS_CIPHER_AES_256_GCM, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_ECDH_RSA,                                  
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_GCM_C */                                                                                           
#endif /* MBEDTLS_SHA512_C */                                                                                        
#endif /* MBEDTLS_AES_C */                                                                                           

#if defined(MBEDTLS_CAMELLIA_C                                                                                      )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_ECDH_RSA_WITH_CAMELLIA_128_CBC_SHA256, "TLS-ECDH-RSA-WITH-CAMELLIA-128-CBC-SHA256",                
      MBEDTLS_CIPHER_CAMELLIA_128_CBC, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_ECDH_RSA,                             
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA256_C */                                                                                        
#if defined(MBEDTLS_SHA512_C                                                                                        )
    { MBEDTLS_TLS_ECDH_RSA_WITH_CAMELLIA_256_CBC_SHA384, "TLS-ECDH-RSA-WITH-CAMELLIA-256-CBC-SHA384",                
      MBEDTLS_CIPHER_CAMELLIA_256_CBC, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_ECDH_RSA,                             
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA512_C */                                                                                        
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 

#if defined(MBEDTLS_GCM_C                                                                                           )
#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_ECDH_RSA_WITH_CAMELLIA_128_GCM_SHA256, "TLS-ECDH-RSA-WITH-CAMELLIA-128-GCM-SHA256",                
      MBEDTLS_CIPHER_CAMELLIA_128_GCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_ECDH_RSA,                             
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA256_C */                                                                                        
#if defined(MBEDTLS_SHA512_C                                                                                        )
    { MBEDTLS_TLS_ECDH_RSA_WITH_CAMELLIA_256_GCM_SHA384, "TLS-ECDH-RSA-WITH-CAMELLIA-256-GCM-SHA384",                
      MBEDTLS_CIPHER_CAMELLIA_256_GCM, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_ECDH_RSA,                             
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA512_C */                                                                                        
#endif /* MBEDTLS_GCM_C */                                                                                           
#endif /* MBEDTLS_CAMELLIA_C */                                                                                      

#if defined(MBEDTLS_DES_C                                                                                           )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA, "TLS-ECDH-RSA-WITH-3DES-EDE-CBC-SHA",                              
      MBEDTLS_CIPHER_DES_EDE3_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_ECDH_RSA,                                   
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#endif /* MBEDTLS_DES_C */                                                                                           

#if defined(MBEDTLS_ARC4_C                                                                                          )
#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_ECDH_RSA_WITH_RC4_128_SHA, "TLS-ECDH-RSA-WITH-RC4-128-SHA",                                        
      MBEDTLS_CIPHER_ARC4_128, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_ECDH_RSA,                                       
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_NODTLS },                                                                                  
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_ARC4_C */                                                                                          

#if defined(MBEDTLS_CIPHER_NULL_CIPHER                                                                              )
#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_ECDH_RSA_WITH_NULL_SHA, "TLS-ECDH-RSA-WITH-NULL-SHA",                                              
      MBEDTLS_CIPHER_NULL, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_ECDH_RSA,                                           
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_WEAK },                                                                                    
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_CIPHER_NULL_CIPHER */                                                                              
#endif /* MBEDTLS_KEY_EXCHANGE_ECDH_RSA_ENABLED */                                                                   

#if defined(MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA_ENABLED                                                                 )
#if defined(MBEDTLS_AES_C                                                                                           )
#if defined(MBEDTLS_SHA1_C                                                                                          )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
    { MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA, "TLS-ECDH-ECDSA-WITH-AES-128-CBC-SHA",                            
      MBEDTLS_CIPHER_AES_128_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA,                                  
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
    { MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA, "TLS-ECDH-ECDSA-WITH-AES-256-CBC-SHA",                            
      MBEDTLS_CIPHER_AES_256_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA,                                  
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#endif /* MBEDTLS_SHA1_C */                                                                                          
#if defined(MBEDTLS_SHA256_C                                                                                        )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
    { MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256, "TLS-ECDH-ECDSA-WITH-AES-128-CBC-SHA256",                      
      MBEDTLS_CIPHER_AES_128_CBC, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA,                                
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#if defined(MBEDTLS_GCM_C                                                                                           )
    { MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256, "TLS-ECDH-ECDSA-WITH-AES-128-GCM-SHA256",                      
      MBEDTLS_CIPHER_AES_128_GCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA,                                
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_GCM_C */                                                                                           
#endif /* MBEDTLS_SHA256_C */                                                                                        
#if defined(MBEDTLS_SHA512_C                                                                                        )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
    { MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384, "TLS-ECDH-ECDSA-WITH-AES-256-CBC-SHA384",                      
      MBEDTLS_CIPHER_AES_256_CBC, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA,                                
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#if defined(MBEDTLS_GCM_C                                                                                           )
    { MBEDTLS_TLS_ECDH_ECDSA_WITH_AES_256_GCM_SHA384, "TLS-ECDH-ECDSA-WITH-AES-256-GCM-SHA384",                      
      MBEDTLS_CIPHER_AES_256_GCM, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA,                                
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_GCM_C */                                                                                           
#endif /* MBEDTLS_SHA512_C */                                                                                        
#endif /* MBEDTLS_AES_C */                                                                                           

#if defined(MBEDTLS_CAMELLIA_C                                                                                      )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_ECDH_ECDSA_WITH_CAMELLIA_128_CBC_SHA256, "TLS-ECDH-ECDSA-WITH-CAMELLIA-128-CBC-SHA256",            
      MBEDTLS_CIPHER_CAMELLIA_128_CBC, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA,                           
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA256_C */                                                                                        
#if defined(MBEDTLS_SHA512_C                                                                                        )
    { MBEDTLS_TLS_ECDH_ECDSA_WITH_CAMELLIA_256_CBC_SHA384, "TLS-ECDH-ECDSA-WITH-CAMELLIA-256-CBC-SHA384",            
      MBEDTLS_CIPHER_CAMELLIA_256_CBC, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA,                           
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA512_C */                                                                                        
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 

#if defined(MBEDTLS_GCM_C                                                                                           )
#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_ECDH_ECDSA_WITH_CAMELLIA_128_GCM_SHA256, "TLS-ECDH-ECDSA-WITH-CAMELLIA-128-GCM-SHA256",            
      MBEDTLS_CIPHER_CAMELLIA_128_GCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA,                           
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA256_C */                                                                                        
#if defined(MBEDTLS_SHA512_C                                                                                        )
    { MBEDTLS_TLS_ECDH_ECDSA_WITH_CAMELLIA_256_GCM_SHA384, "TLS-ECDH-ECDSA-WITH-CAMELLIA-256-GCM-SHA384",            
      MBEDTLS_CIPHER_CAMELLIA_256_GCM, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA,                           
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA512_C */                                                                                        
#endif /* MBEDTLS_GCM_C */                                                                                           
#endif /* MBEDTLS_CAMELLIA_C */                                                                                      

#if defined(MBEDTLS_DES_C                                                                                           )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA, "TLS-ECDH-ECDSA-WITH-3DES-EDE-CBC-SHA",                          
      MBEDTLS_CIPHER_DES_EDE3_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA,                                 
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#endif /* MBEDTLS_DES_C */                                                                                           

#if defined(MBEDTLS_ARC4_C                                                                                          )
#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_ECDH_ECDSA_WITH_RC4_128_SHA, "TLS-ECDH-ECDSA-WITH-RC4-128-SHA",                                    
      MBEDTLS_CIPHER_ARC4_128, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA,                                     
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_NODTLS },                                                                                  
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_ARC4_C */                                                                                          

#if defined(MBEDTLS_CIPHER_NULL_CIPHER                                                                              )
#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_ECDH_ECDSA_WITH_NULL_SHA, "TLS-ECDH-ECDSA-WITH-NULL-SHA",                                          
      MBEDTLS_CIPHER_NULL, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA,                                         
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_WEAK },                                                                                    
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_CIPHER_NULL_CIPHER */                                                                              
#endif /* MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA_ENABLED */                                                                 

#if defined(MBEDTLS_KEY_EXCHANGE_PSK_ENABLED                                                                        )
#if defined(MBEDTLS_AES_C                                                                                           )
#if defined(MBEDTLS_GCM_C                                                                                           )
#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_PSK_WITH_AES_128_GCM_SHA256, "TLS-PSK-WITH-AES-128-GCM-SHA256",                                    
      MBEDTLS_CIPHER_AES_128_GCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_PSK,                                       
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA256_C */                                                                                        

#if defined(MBEDTLS_SHA512_C                                                                                        )
    { MBEDTLS_TLS_PSK_WITH_AES_256_GCM_SHA384, "TLS-PSK-WITH-AES-256-GCM-SHA384",                                    
      MBEDTLS_CIPHER_AES_256_GCM, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_PSK,                                       
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA512_C */                                                                                        
#endif /* MBEDTLS_GCM_C */                                                                                           

#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_PSK_WITH_AES_128_CBC_SHA256, "TLS-PSK-WITH-AES-128-CBC-SHA256",                                    
      MBEDTLS_CIPHER_AES_128_CBC, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_PSK,                                       
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA256_C */                                                                                        

#if defined(MBEDTLS_SHA512_C                                                                                        )
    { MBEDTLS_TLS_PSK_WITH_AES_256_CBC_SHA384, "TLS-PSK-WITH-AES-256-CBC-SHA384",                                    
      MBEDTLS_CIPHER_AES_256_CBC, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_PSK,                                       
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA512_C */                                                                                        

#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_PSK_WITH_AES_128_CBC_SHA, "TLS-PSK-WITH-AES-128-CBC-SHA",                                          
      MBEDTLS_CIPHER_AES_128_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_PSK,                                         
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_0,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           

    { MBEDTLS_TLS_PSK_WITH_AES_256_CBC_SHA, "TLS-PSK-WITH-AES-256-CBC-SHA",                                          
      MBEDTLS_CIPHER_AES_256_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_PSK,                                         
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_0,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#if defined(MBEDTLS_CCM_C                                                                                           )
    { MBEDTLS_TLS_PSK_WITH_AES_256_CCM, "TLS-PSK-WITH-AES-256-CCM",                                                  
      MBEDTLS_CIPHER_AES_256_CCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_PSK,                                       
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
    { MBEDTLS_TLS_PSK_WITH_AES_256_CCM_8, "TLS-PSK-WITH-AES-256-CCM-8",                                              
      MBEDTLS_CIPHER_AES_256_CCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_PSK,                                       
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_SHORT_TAG },                                                                               
    { MBEDTLS_TLS_PSK_WITH_AES_128_CCM, "TLS-PSK-WITH-AES-128-CCM",                                                  
      MBEDTLS_CIPHER_AES_128_CCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_PSK,                                       
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
    { MBEDTLS_TLS_PSK_WITH_AES_128_CCM_8, "TLS-PSK-WITH-AES-128-CCM-8",                                              
      MBEDTLS_CIPHER_AES_128_CCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_PSK,                                       
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_SHORT_TAG },                                                                               
#endif /* MBEDTLS_CCM_C */                                                                                           
#endif /* MBEDTLS_AES_C */                                                                                           

#if defined(MBEDTLS_CAMELLIA_C                                                                                      )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_PSK_WITH_CAMELLIA_128_CBC_SHA256, "TLS-PSK-WITH-CAMELLIA-128-CBC-SHA256",                          
      MBEDTLS_CIPHER_CAMELLIA_128_CBC, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_PSK,                                  
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA256_C */                                                                                        

#if defined(MBEDTLS_SHA512_C                                                                                        )
    { MBEDTLS_TLS_PSK_WITH_CAMELLIA_256_CBC_SHA384, "TLS-PSK-WITH-CAMELLIA-256-CBC-SHA384",                          
      MBEDTLS_CIPHER_CAMELLIA_256_CBC, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_PSK,                                  
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA512_C */                                                                                        
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 

#if defined(MBEDTLS_GCM_C                                                                                           )
#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_PSK_WITH_CAMELLIA_128_GCM_SHA256, "TLS-PSK-WITH-CAMELLIA-128-GCM-SHA256",                          
      MBEDTLS_CIPHER_CAMELLIA_128_GCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_PSK,                                  
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA256_C */                                                                                        

#if defined(MBEDTLS_SHA512_C                                                                                        )
    { MBEDTLS_TLS_PSK_WITH_CAMELLIA_256_GCM_SHA384, "TLS-PSK-WITH-CAMELLIA-256-GCM-SHA384",                          
      MBEDTLS_CIPHER_CAMELLIA_256_GCM, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_PSK,                                  
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA512_C */                                                                                        
#endif /* MBEDTLS_GCM_C */                                                                                           
#endif /* MBEDTLS_CAMELLIA_C */                                                                                      

#if defined(MBEDTLS_DES_C                                                                                           )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_PSK_WITH_3DES_EDE_CBC_SHA, "TLS-PSK-WITH-3DES-EDE-CBC-SHA",                                        
      MBEDTLS_CIPHER_DES_EDE3_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_PSK,                                        
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_0,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#endif /* MBEDTLS_DES_C */                                                                                           

#if defined(MBEDTLS_ARC4_C                                                                                          )
#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_PSK_WITH_RC4_128_SHA, "TLS-PSK-WITH-RC4-128-SHA",                                                  
      MBEDTLS_CIPHER_ARC4_128, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_PSK,                                            
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_0,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_NODTLS },                                                                                  
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_ARC4_C */                                                                                          
#endif /* MBEDTLS_KEY_EXCHANGE_PSK_ENABLED */                                                                        

#if defined(MBEDTLS_KEY_EXCHANGE_DHE_PSK_ENABLED                                                                    )
#if defined(MBEDTLS_AES_C                                                                                           )
#if defined(MBEDTLS_GCM_C                                                                                           )
#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_DHE_PSK_WITH_AES_128_GCM_SHA256, "TLS-DHE-PSK-WITH-AES-128-GCM-SHA256",                            
      MBEDTLS_CIPHER_AES_128_GCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_DHE_PSK,                                   
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA256_C */                                                                                        

#if defined(MBEDTLS_SHA512_C                                                                                        )
    { MBEDTLS_TLS_DHE_PSK_WITH_AES_256_GCM_SHA384, "TLS-DHE-PSK-WITH-AES-256-GCM-SHA384",                            
      MBEDTLS_CIPHER_AES_256_GCM, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_DHE_PSK,                                   
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA512_C */                                                                                        
#endif /* MBEDTLS_GCM_C */                                                                                           

#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_DHE_PSK_WITH_AES_128_CBC_SHA256, "TLS-DHE-PSK-WITH-AES-128-CBC-SHA256",                            
      MBEDTLS_CIPHER_AES_128_CBC, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_DHE_PSK,                                   
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA256_C */                                                                                        

#if defined(MBEDTLS_SHA512_C                                                                                        )
    { MBEDTLS_TLS_DHE_PSK_WITH_AES_256_CBC_SHA384, "TLS-DHE-PSK-WITH-AES-256-CBC-SHA384",                            
      MBEDTLS_CIPHER_AES_256_CBC, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_DHE_PSK,                                   
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA512_C */                                                                                        

#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_DHE_PSK_WITH_AES_128_CBC_SHA, "TLS-DHE-PSK-WITH-AES-128-CBC-SHA",                                  
      MBEDTLS_CIPHER_AES_128_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_DHE_PSK,                                     
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_0,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           

    { MBEDTLS_TLS_DHE_PSK_WITH_AES_256_CBC_SHA, "TLS-DHE-PSK-WITH-AES-256-CBC-SHA",                                  
      MBEDTLS_CIPHER_AES_256_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_DHE_PSK,                                     
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_0,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#if defined(MBEDTLS_CCM_C                                                                                           )
    { MBEDTLS_TLS_DHE_PSK_WITH_AES_256_CCM, "TLS-DHE-PSK-WITH-AES-256-CCM",                                          
      MBEDTLS_CIPHER_AES_256_CCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_DHE_PSK,                                   
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
    { MBEDTLS_TLS_DHE_PSK_WITH_AES_256_CCM_8, "TLS-DHE-PSK-WITH-AES-256-CCM-8",                                      
      MBEDTLS_CIPHER_AES_256_CCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_DHE_PSK,                                   
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_SHORT_TAG },                                                                               
    { MBEDTLS_TLS_DHE_PSK_WITH_AES_128_CCM, "TLS-DHE-PSK-WITH-AES-128-CCM",                                          
      MBEDTLS_CIPHER_AES_128_CCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_DHE_PSK,                                   
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
    { MBEDTLS_TLS_DHE_PSK_WITH_AES_128_CCM_8, "TLS-DHE-PSK-WITH-AES-128-CCM-8",                                      
      MBEDTLS_CIPHER_AES_128_CCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_DHE_PSK,                                   
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_SHORT_TAG },                                                                               
#endif /* MBEDTLS_CCM_C */                                                                                           
#endif /* MBEDTLS_AES_C */                                                                                           

#if defined(MBEDTLS_CAMELLIA_C                                                                                      )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_DHE_PSK_WITH_CAMELLIA_128_CBC_SHA256, "TLS-DHE-PSK-WITH-CAMELLIA-128-CBC-SHA256",                  
      MBEDTLS_CIPHER_CAMELLIA_128_CBC, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_DHE_PSK,                              
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA256_C */                                                                                        

#if defined(MBEDTLS_SHA512_C                                                                                        )
    { MBEDTLS_TLS_DHE_PSK_WITH_CAMELLIA_256_CBC_SHA384, "TLS-DHE-PSK-WITH-CAMELLIA-256-CBC-SHA384",                  
      MBEDTLS_CIPHER_CAMELLIA_256_CBC, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_DHE_PSK,                              
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA512_C */                                                                                        
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 

#if defined(MBEDTLS_GCM_C                                                                                           )
#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_DHE_PSK_WITH_CAMELLIA_128_GCM_SHA256, "TLS-DHE-PSK-WITH-CAMELLIA-128-GCM-SHA256",                  
      MBEDTLS_CIPHER_CAMELLIA_128_GCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_DHE_PSK,                              
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA256_C */                                                                                        

#if defined(MBEDTLS_SHA512_C                                                                                        )
    { MBEDTLS_TLS_DHE_PSK_WITH_CAMELLIA_256_GCM_SHA384, "TLS-DHE-PSK-WITH-CAMELLIA-256-GCM-SHA384",                  
      MBEDTLS_CIPHER_CAMELLIA_256_GCM, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_DHE_PSK,                              
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA512_C */                                                                                        
#endif /* MBEDTLS_GCM_C */                                                                                           
#endif /* MBEDTLS_CAMELLIA_C */                                                                                      

#if defined(MBEDTLS_DES_C                                                                                           )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_DHE_PSK_WITH_3DES_EDE_CBC_SHA, "TLS-DHE-PSK-WITH-3DES-EDE-CBC-SHA",                                
      MBEDTLS_CIPHER_DES_EDE3_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_DHE_PSK,                                    
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_0,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#endif /* MBEDTLS_DES_C */                                                                                           

#if defined(MBEDTLS_ARC4_C                                                                                          )
#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_DHE_PSK_WITH_RC4_128_SHA, "TLS-DHE-PSK-WITH-RC4-128-SHA",                                          
      MBEDTLS_CIPHER_ARC4_128, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_DHE_PSK,                                        
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_0,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_NODTLS },                                                                                  
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_ARC4_C */                                                                                          
#endif /* MBEDTLS_KEY_EXCHANGE_DHE_PSK_ENABLED */                                                                    

#if defined(MBEDTLS_KEY_EXCHANGE_ECDHE_PSK_ENABLED                                                                  )
#if defined(MBEDTLS_AES_C                                                                                           )

#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_ECDHE_PSK_WITH_AES_128_CBC_SHA256, "TLS-ECDHE-PSK-WITH-AES-128-CBC-SHA256",                        
      MBEDTLS_CIPHER_AES_128_CBC, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_ECDHE_PSK,                                 
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA256_C */                                                                                        

#if defined(MBEDTLS_SHA512_C                                                                                        )
    { MBEDTLS_TLS_ECDHE_PSK_WITH_AES_256_CBC_SHA384, "TLS-ECDHE-PSK-WITH-AES-256-CBC-SHA384",                        
      MBEDTLS_CIPHER_AES_256_CBC, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_ECDHE_PSK,                                 
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA512_C */                                                                                        

#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_ECDHE_PSK_WITH_AES_128_CBC_SHA, "TLS-ECDHE-PSK-WITH-AES-128-CBC-SHA",                              
      MBEDTLS_CIPHER_AES_128_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_ECDHE_PSK,                                   
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           

    { MBEDTLS_TLS_ECDHE_PSK_WITH_AES_256_CBC_SHA, "TLS-ECDHE-PSK-WITH-AES-256-CBC-SHA",                              
      MBEDTLS_CIPHER_AES_256_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_ECDHE_PSK,                                   
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#endif /* MBEDTLS_AES_C */                                                                                           

#if defined(MBEDTLS_CAMELLIA_C                                                                                      )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_ECDHE_PSK_WITH_CAMELLIA_128_CBC_SHA256, "TLS-ECDHE-PSK-WITH-CAMELLIA-128-CBC-SHA256",              
      MBEDTLS_CIPHER_CAMELLIA_128_CBC, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_ECDHE_PSK,                            
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA256_C */                                                                                        

#if defined(MBEDTLS_SHA512_C                                                                                        )
    { MBEDTLS_TLS_ECDHE_PSK_WITH_CAMELLIA_256_CBC_SHA384, "TLS-ECDHE-PSK-WITH-CAMELLIA-256-CBC-SHA384",              
      MBEDTLS_CIPHER_CAMELLIA_256_CBC, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_ECDHE_PSK,                            
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA512_C */                                                                                        
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#endif /* MBEDTLS_CAMELLIA_C */                                                                                      

#if defined(MBEDTLS_DES_C                                                                                           )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_ECDHE_PSK_WITH_3DES_EDE_CBC_SHA, "TLS-ECDHE-PSK-WITH-3DES-EDE-CBC-SHA",                            
      MBEDTLS_CIPHER_DES_EDE3_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_ECDHE_PSK,                                  
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#endif /* MBEDTLS_DES_C */                                                                                           

#if defined(MBEDTLS_ARC4_C                                                                                          )
#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_ECDHE_PSK_WITH_RC4_128_SHA, "TLS-ECDHE-PSK-WITH-RC4-128-SHA",                                      
      MBEDTLS_CIPHER_ARC4_128, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_ECDHE_PSK,                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_NODTLS },                                                                                  
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_ARC4_C */                                                                                          
#endif /* MBEDTLS_KEY_EXCHANGE_ECDHE_PSK_ENABLED */                                                                  

#if defined(MBEDTLS_KEY_EXCHANGE_RSA_PSK_ENABLED                                                                    )
#if defined(MBEDTLS_AES_C                                                                                           )
#if defined(MBEDTLS_GCM_C                                                                                           )
#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_RSA_PSK_WITH_AES_128_GCM_SHA256, "TLS-RSA-PSK-WITH-AES-128-GCM-SHA256",                            
      MBEDTLS_CIPHER_AES_128_GCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_RSA_PSK,                                   
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA256_C */                                                                                        

#if defined(MBEDTLS_SHA512_C                                                                                        )
    { MBEDTLS_TLS_RSA_PSK_WITH_AES_256_GCM_SHA384, "TLS-RSA-PSK-WITH-AES-256-GCM-SHA384",                            
      MBEDTLS_CIPHER_AES_256_GCM, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_RSA_PSK,                                   
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA512_C */                                                                                        
#endif /* MBEDTLS_GCM_C */                                                                                           

#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_RSA_PSK_WITH_AES_128_CBC_SHA256, "TLS-RSA-PSK-WITH-AES-128-CBC-SHA256",                            
      MBEDTLS_CIPHER_AES_128_CBC, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_RSA_PSK,                                   
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA256_C */                                                                                        

#if defined(MBEDTLS_SHA512_C                                                                                        )
    { MBEDTLS_TLS_RSA_PSK_WITH_AES_256_CBC_SHA384, "TLS-RSA-PSK-WITH-AES-256-CBC-SHA384",                            
      MBEDTLS_CIPHER_AES_256_CBC, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_RSA_PSK,                                   
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA512_C */                                                                                        

#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_RSA_PSK_WITH_AES_128_CBC_SHA, "TLS-RSA-PSK-WITH-AES-128-CBC-SHA",                                  
      MBEDTLS_CIPHER_AES_128_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_RSA_PSK,                                     
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           

    { MBEDTLS_TLS_RSA_PSK_WITH_AES_256_CBC_SHA, "TLS-RSA-PSK-WITH-AES-256-CBC-SHA",                                  
      MBEDTLS_CIPHER_AES_256_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_RSA_PSK,                                     
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#endif /* MBEDTLS_AES_C */                                                                                           

#if defined(MBEDTLS_CAMELLIA_C                                                                                      )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_RSA_PSK_WITH_CAMELLIA_128_CBC_SHA256, "TLS-RSA-PSK-WITH-CAMELLIA-128-CBC-SHA256",                  
      MBEDTLS_CIPHER_CAMELLIA_128_CBC, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_RSA_PSK,                              
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA256_C */                                                                                        

#if defined(MBEDTLS_SHA512_C                                                                                        )
    { MBEDTLS_TLS_RSA_PSK_WITH_CAMELLIA_256_CBC_SHA384, "TLS-RSA-PSK-WITH-CAMELLIA-256-CBC-SHA384",                  
      MBEDTLS_CIPHER_CAMELLIA_256_CBC, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_RSA_PSK,                              
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA512_C */                                                                                        
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 

#if defined(MBEDTLS_GCM_C                                                                                           )
#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_RSA_PSK_WITH_CAMELLIA_128_GCM_SHA256, "TLS-RSA-PSK-WITH-CAMELLIA-128-GCM-SHA256",                  
      MBEDTLS_CIPHER_CAMELLIA_128_GCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_RSA_PSK,                              
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA256_C */                                                                                        

#if defined(MBEDTLS_SHA512_C                                                                                        )
    { MBEDTLS_TLS_RSA_PSK_WITH_CAMELLIA_256_GCM_SHA384, "TLS-RSA-PSK-WITH-CAMELLIA-256-GCM-SHA384",                  
      MBEDTLS_CIPHER_CAMELLIA_256_GCM, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_RSA_PSK,                              
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA512_C */                                                                                        
#endif /* MBEDTLS_GCM_C */                                                                                           
#endif /* MBEDTLS_CAMELLIA_C */                                                                                      

#if defined(MBEDTLS_DES_C                                                                                           )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_RSA_PSK_WITH_3DES_EDE_CBC_SHA, "TLS-RSA-PSK-WITH-3DES-EDE-CBC-SHA",                                
      MBEDTLS_CIPHER_DES_EDE3_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_RSA_PSK,                                    
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      0 },                                                                                                           
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#endif /* MBEDTLS_DES_C */                                                                                           

#if defined(MBEDTLS_ARC4_C                                                                                          )
#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_RSA_PSK_WITH_RC4_128_SHA, "TLS-RSA-PSK-WITH-RC4-128-SHA",                                          
      MBEDTLS_CIPHER_ARC4_128, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_RSA_PSK,                                        
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_NODTLS },                                                                                  
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_ARC4_C */                                                                                          
#endif /* MBEDTLS_KEY_EXCHANGE_RSA_PSK_ENABLED */                                                                    

#if defined(MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED                                                                    )
#if defined(MBEDTLS_AES_C                                                                                           )
#if defined(MBEDTLS_CCM_C                                                                                           )
    { MBEDTLS_TLS_ECJPAKE_WITH_AES_128_CCM_8, "TLS-ECJPAKE-WITH-AES-128-CCM-8",                                      
      MBEDTLS_CIPHER_AES_128_CCM, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_ECJPAKE,                                   
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_SHORT_TAG },                                                                               
#endif /* MBEDTLS_CCM_C */                                                                                           
#endif /* MBEDTLS_AES_C */                                                                                           
#endif /* MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED */                                                                    

#if defined(MBEDTLS_ENABLE_WEAK_CIPHERSUITES                                                                        )
#if defined(MBEDTLS_CIPHER_NULL_CIPHER                                                                              )
#if defined(MBEDTLS_KEY_EXCHANGE_RSA_ENABLED                                                                        )
#if defined(MBEDTLS_MD5_C                                                                                           )
    { MBEDTLS_TLS_RSA_WITH_NULL_MD5, "TLS-RSA-WITH-NULL-MD5",                                                        
      MBEDTLS_CIPHER_NULL, MBEDTLS_MD_MD5, MBEDTLS_KEY_EXCHANGE_RSA,                                                 
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_0,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_WEAK },                                                                                    
#endif                                                                                                               

#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_RSA_WITH_NULL_SHA, "TLS-RSA-WITH-NULL-SHA",                                                        
      MBEDTLS_CIPHER_NULL, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_RSA,                                                
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_0,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_WEAK },                                                                                    
#endif                                                                                                               

#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_RSA_WITH_NULL_SHA256, "TLS-RSA-WITH-NULL-SHA256",                                                  
      MBEDTLS_CIPHER_NULL, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_RSA,                                              
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_WEAK },                                                                                    
#endif                                                                                                               
#endif /* MBEDTLS_KEY_EXCHANGE_RSA_ENABLED */                                                                        

#if defined(MBEDTLS_KEY_EXCHANGE_PSK_ENABLED                                                                        )
#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_PSK_WITH_NULL_SHA, "TLS-PSK-WITH-NULL-SHA",                                                        
      MBEDTLS_CIPHER_NULL, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_PSK,                                                
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_0,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_WEAK },                                                                                    
#endif /* MBEDTLS_SHA1_C */                                                                                          

#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_PSK_WITH_NULL_SHA256, "TLS-PSK-WITH-NULL-SHA256",                                                  
      MBEDTLS_CIPHER_NULL, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_PSK,                                              
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_WEAK },                                                                                    
#endif                                                                                                               

#if defined(MBEDTLS_SHA512_C                                                                                        )
    { MBEDTLS_TLS_PSK_WITH_NULL_SHA384, "TLS-PSK-WITH-NULL-SHA384",                                                  
      MBEDTLS_CIPHER_NULL, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_PSK,                                              
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_WEAK },                                                                                    
#endif                                                                                                               
#endif /* MBEDTLS_KEY_EXCHANGE_PSK_ENABLED */                                                                        

#if defined(MBEDTLS_KEY_EXCHANGE_DHE_PSK_ENABLED                                                                    )
#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_DHE_PSK_WITH_NULL_SHA, "TLS-DHE-PSK-WITH-NULL-SHA",                                                
      MBEDTLS_CIPHER_NULL, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_DHE_PSK,                                            
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_0,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_WEAK },                                                                                    
#endif /* MBEDTLS_SHA1_C */                                                                                          

#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_DHE_PSK_WITH_NULL_SHA256, "TLS-DHE-PSK-WITH-NULL-SHA256",                                          
      MBEDTLS_CIPHER_NULL, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_DHE_PSK,                                          
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_WEAK },                                                                                    
#endif                                                                                                               

#if defined(MBEDTLS_SHA512_C                                                                                        )
    { MBEDTLS_TLS_DHE_PSK_WITH_NULL_SHA384, "TLS-DHE-PSK-WITH-NULL-SHA384",                                          
      MBEDTLS_CIPHER_NULL, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_DHE_PSK,                                          
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_WEAK },                                                                                    
#endif                                                                                                               
#endif /* MBEDTLS_KEY_EXCHANGE_DHE_PSK_ENABLED */                                                                    

#if defined(MBEDTLS_KEY_EXCHANGE_ECDHE_PSK_ENABLED                                                                  )
#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_ECDHE_PSK_WITH_NULL_SHA, "TLS-ECDHE-PSK-WITH-NULL-SHA",                                            
      MBEDTLS_CIPHER_NULL, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_ECDHE_PSK,                                          
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_WEAK },                                                                                    
#endif /* MBEDTLS_SHA1_C */                                                                                          

#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_ECDHE_PSK_WITH_NULL_SHA256, "TLS-ECDHE-PSK-WITH-NULL-SHA256",                                      
      MBEDTLS_CIPHER_NULL, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_ECDHE_PSK,                                        
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_WEAK },                                                                                    
#endif                                                                                                               

#if defined(MBEDTLS_SHA512_C                                                                                        )
    { MBEDTLS_TLS_ECDHE_PSK_WITH_NULL_SHA384, "TLS-ECDHE-PSK-WITH-NULL-SHA384",                                      
      MBEDTLS_CIPHER_NULL, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_ECDHE_PSK,                                        
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_WEAK },                                                                                    
#endif                                                                                                               
#endif /* MBEDTLS_KEY_EXCHANGE_ECDHE_PSK_ENABLED */                                                                  

#if defined(MBEDTLS_KEY_EXCHANGE_RSA_PSK_ENABLED                                                                    )
#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_RSA_PSK_WITH_NULL_SHA, "TLS-RSA-PSK-WITH-NULL-SHA",                                                
      MBEDTLS_CIPHER_NULL, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_RSA_PSK,                                            
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_WEAK },                                                                                    
#endif /* MBEDTLS_SHA1_C */                                                                                          

#if defined(MBEDTLS_SHA256_C                                                                                        )
    { MBEDTLS_TLS_RSA_PSK_WITH_NULL_SHA256, "TLS-RSA-PSK-WITH-NULL-SHA256",                                          
      MBEDTLS_CIPHER_NULL, MBEDTLS_MD_SHA256, MBEDTLS_KEY_EXCHANGE_RSA_PSK,                                          
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_WEAK },                                                                                    
#endif                                                                                                               

#if defined(MBEDTLS_SHA512_C                                                                                        )
    { MBEDTLS_TLS_RSA_PSK_WITH_NULL_SHA384, "TLS-RSA-PSK-WITH-NULL-SHA384",                                          
      MBEDTLS_CIPHER_NULL, MBEDTLS_MD_SHA384, MBEDTLS_KEY_EXCHANGE_RSA_PSK,                                          
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_1,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_WEAK },                                                                                    
#endif                                                                                                               
#endif /* MBEDTLS_KEY_EXCHANGE_RSA_PSK_ENABLED */                                                                    
#endif /* MBEDTLS_CIPHER_NULL_CIPHER */                                                                              

#if defined(MBEDTLS_DES_C                                                                                           )
#if defined(MBEDTLS_CIPHER_MODE_CBC                                                                                 )
#if defined(MBEDTLS_KEY_EXCHANGE_DHE_RSA_ENABLED                                                                    )
#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_DHE_RSA_WITH_DES_CBC_SHA, "TLS-DHE-RSA-WITH-DES-CBC-SHA",                                          
      MBEDTLS_CIPHER_DES_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_DHE_RSA,                                         
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_0,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_WEAK },                                                                                    
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_KEY_EXCHANGE_DHE_RSA_ENABLED */                                                                    

#if defined(MBEDTLS_KEY_EXCHANGE_RSA_ENABLED                                                                        )
#if defined(MBEDTLS_SHA1_C                                                                                          )
    { MBEDTLS_TLS_RSA_WITH_DES_CBC_SHA, "TLS-RSA-WITH-DES-CBC-SHA",                                                  
      MBEDTLS_CIPHER_DES_CBC, MBEDTLS_MD_SHA1, MBEDTLS_KEY_EXCHANGE_RSA,                                             
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_0,                                                      
      MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3,                                                      
      MBEDTLS_CIPHERSUITE_WEAK },                                                                                    
#endif /* MBEDTLS_SHA1_C */                                                                                          
#endif /* MBEDTLS_KEY_EXCHANGE_RSA_ENABLED */                                                                        
#endif /* MBEDTLS_CIPHER_MODE_CBC */                                                                                 
#endif /* MBEDTLS_DES_C */                                                                                           
#endif /* MBEDTLS_ENABLE_WEAK_CIPHERSUITES */                                                                        

    { 0, "",                                                                                                         
      MBEDTLS_CIPHER_NONE, MBEDTLS_MD_NONE, MBEDTLS_KEY_EXCHANGE_NONE,                                               
      0, 0, 0, 0, 0                                                                                                 }
};

#if defined(MBEDTLS_SSL_CIPHERSUITES                                                                                )
const int *mbedtls_ssl_list_ciphersuites( void                                                                      )
{
    return( ciphersuite_preference                                                                                 );
}
#else                                                                                                                
#define MAX_CIPHERSUITES    sizeof( ciphersuite_definitions     ) /         \                                        
                            sizeof( ciphersuite_definitions[0]                                                      )
static int supported_ciphersuites[MAX_CIPHERSUITES]                                                                 ;
static int supported_init = 0                                                                                       ;

const int *mbedtls_ssl_list_ciphersuites( void                                                                      )
{
    /*                                                                                                               
     * On initial call filter out all ciphersuites not supported by current                                          
     * build based on presence in the ciphersuite_definitions.                                                       
     */                                                                                                              
    if( supported_init == 0                                                                                         )
    {
        const int *p                                                                                                ;
        int *q                                                                                                      ;

        for( p = ciphersuite_preference, q = supported_ciphersuites                                                 ;
             *p != 0 && q < supported_ciphersuites + MAX_CIPHERSUITES - 1                                           ;
             p++                                                                                                    )
        {
#if defined(MBEDTLS_REMOVE_ARC4_CIPHERSUITES                                                                        )
            const mbedtls_ssl_ciphersuite_t *cs_info                                                                ;
            if( ( cs_info = mbedtls_ssl_ciphersuite_from_id( *p ) ) != NULL &&                                       
                cs_info->cipher != MBEDTLS_CIPHER_ARC4_128                                                          )
#else                                                                                                                
            if( mbedtls_ssl_ciphersuite_from_id( *p ) != NULL                                                       )
#endif                                                                                                               
                *(q++) = *p                                                                                         ;
        }
        *q = 0                                                                                                      ;

        supported_init = 1                                                                                          ;
    }

    return( supported_ciphersuites                                                                                 );
}
#endif /* MBEDTLS_SSL_CIPHERSUITES */                                                                                

const mbedtls_ssl_ciphersuite_t *mbedtls_ssl_ciphersuite_from_string(                                                
                                                const char *ciphersuite_name                                        )
{
    const mbedtls_ssl_ciphersuite_t *cur = ciphersuite_definitions                                                  ;

    if( NULL == ciphersuite_name                                                                                    )
        return( NULL                                                                                               );

    while( cur->id != 0                                                                                             )
    {
        if( 0 == strcmp( cur->name, ciphersuite_name                                                              ) )
            return( cur                                                                                            );

        cur++                                                                                                       ;
    }

    return( NULL                                                                                                   );
}

const mbedtls_ssl_ciphersuite_t *mbedtls_ssl_ciphersuite_from_id( int ciphersuite                                   )
{
    const mbedtls_ssl_ciphersuite_t *cur = ciphersuite_definitions                                                  ;

    while( cur->id != 0                                                                                             )
    {
        if( cur->id == ciphersuite                                                                                  )
            return( cur                                                                                            );

        cur++                                                                                                       ;
    }

    return( NULL                                                                                                   );
}

const char *mbedtls_ssl_get_ciphersuite_name( const int ciphersuite_id                                              )
{
    const mbedtls_ssl_ciphersuite_t *cur                                                                            ;

    cur = mbedtls_ssl_ciphersuite_from_id( ciphersuite_id                                                          );

    if( cur == NULL                                                                                                 )
        return( "unknown"                                                                                          );

    return( cur->name                                                                                              );
}

int mbedtls_ssl_get_ciphersuite_id( const char *ciphersuite_name                                                    )
{
    const mbedtls_ssl_ciphersuite_t *cur                                                                            ;

    cur = mbedtls_ssl_ciphersuite_from_string( ciphersuite_name                                                    );

    if( cur == NULL                                                                                                 )
        return( 0                                                                                                  );

    return( cur->id                                                                                                );
}

#if defined(MBEDTLS_PK_C                                                                                            )
mbedtls_pk_type_t mbedtls_ssl_get_ciphersuite_sig_pk_alg( const mbedtls_ssl_ciphersuite_t *info                     )
{
    switch( info->key_exchange                                                                                      )
    {
        case MBEDTLS_KEY_EXCHANGE_RSA:                                                                               
        case MBEDTLS_KEY_EXCHANGE_DHE_RSA:                                                                           
        case MBEDTLS_KEY_EXCHANGE_ECDHE_RSA:                                                                         
        case MBEDTLS_KEY_EXCHANGE_RSA_PSK:                                                                           
            return( MBEDTLS_PK_RSA                                                                                 );

        case MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA:                                                                       
            return( MBEDTLS_PK_ECDSA                                                                               );

        case MBEDTLS_KEY_EXCHANGE_ECDH_RSA:                                                                          
        case MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA:                                                                        
            return( MBEDTLS_PK_ECKEY                                                                               );

        default:                                                                                                     
            return( MBEDTLS_PK_NONE                                                                                );
    }
}
#endif /* MBEDTLS_PK_C */                                                                                            

#if defined(MBEDTLS_ECDH_C) || defined(MBEDTLS_ECDSA_C                                                              )
int mbedtls_ssl_ciphersuite_uses_ec( const mbedtls_ssl_ciphersuite_t *info                                          )
{
    switch( info->key_exchange                                                                                      )
    {
        case MBEDTLS_KEY_EXCHANGE_ECDHE_RSA:                                                                         
        case MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA:                                                                       
        case MBEDTLS_KEY_EXCHANGE_ECDHE_PSK:                                                                         
        case MBEDTLS_KEY_EXCHANGE_ECDH_RSA:                                                                          
        case MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA:                                                                        
            return( 1                                                                                              );

        default:                                                                                                     
            return( 0                                                                                              );
    }
}
#endif /* MBEDTLS_ECDH_C || MBEDTLS_ECDSA_C */                                                                       

#if defined(MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED                                                                 )
int mbedtls_ssl_ciphersuite_uses_psk( const mbedtls_ssl_ciphersuite_t *info                                         )
{
    switch( info->key_exchange                                                                                      )
    {
        case MBEDTLS_KEY_EXCHANGE_PSK:                                                                               
        case MBEDTLS_KEY_EXCHANGE_RSA_PSK:                                                                           
        case MBEDTLS_KEY_EXCHANGE_DHE_PSK:                                                                           
        case MBEDTLS_KEY_EXCHANGE_ECDHE_PSK:                                                                         
            return( 1                                                                                              );

        default:                                                                                                     
            return( 0                                                                                              );
    }
}
#endif /* MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED */                                                                 

#endif /* MBEDTLS_SSL_TLS_C */                                                                                       
