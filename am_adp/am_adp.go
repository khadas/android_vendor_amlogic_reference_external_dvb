package am_adp

import (
    "android/soong/android"
    "android/soong/cc"
    "strings"
    "fmt"
)

func init() {
    android.RegisterModuleType("am_adp_defaults", am_adp_defaults)
}

func am_adp_defaults() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, func(ctx android.LoadHookContext) {
        type props struct {
            Exclude_srcs []string
            Cflags []string
        }
        p := &props{}

        vconfig := ctx.Config().VendorConfig("amlogic_vendorconfig")
        adtv_config := vconfig.String("adtv_config")

        fmt.Println("ADTV config:", adtv_config)
        if (strings.Contains(adtv_config, "disable_atsc")) {
            fmt.Println("atsc code disabled in am_adp")
            p.Cflags = append(p.Cflags, "-DDISABLE_ATSC")
            atsc_srcs := []string{
                "am_open_lib/libdvbsi/tables/atsc_eit.c",
                "am_open_lib/libdvbsi/tables/atsc_ett.c",
                "am_open_lib/libdvbsi/tables/atsc_mgt.c",
                "am_open_lib/libdvbsi/tables/atsc_stt.c",
                "am_open_lib/libdvbsi/tables/atsc_vct.c",
                "am_open_lib/libdvbsi/tables/atsc_cea.c",
                "am_open_lib/libdvbsi/tables/huffman_decode.c",
            }
            p.Exclude_srcs = append(p.Exclude_srcs, atsc_srcs...)

        } else {
            //fmt.Println("atsc code enabled in am_adp")
        }

        ctx.AppendProperties(p)
    })
    return module
}

