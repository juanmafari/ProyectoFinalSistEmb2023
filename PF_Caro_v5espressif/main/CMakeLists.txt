idf_component_register(SRCS "softap_example_main.c"
                    INCLUDE_DIRS "."

                    REQUIRES "${requires}"
                    PRIV_REQUIRES "${priv_requires}"
                    EMBED_FILES "web.html")
