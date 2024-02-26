package am_mw

import (
    "android/soong/android"
    "android/soong/cc"
    "strings"
    "fmt"
)

func init() {
    android.RegisterModuleType("am_mw_lib_defaults", am_mw_lib_defaults)
    android.RegisterModuleType("am_mw_defaults", am_mw_defaults)
}

func am_mw_lib_defaults() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, func(ctx android.LoadHookContext) {
        type props struct {
            Shared_libs []string
            Static_libs []string
        }
        p := &props{}

        vconfig := ctx.Config().VendorConfig("amlogic_vendorconfig")
        adtv_config := vconfig.String("adtv_config")

        if (!strings.Contains(adtv_config, "disable_local_db")) {
            p.Static_libs = append(p.Static_libs, "libsqlite")
        }

        if (!strings.Contains(adtv_config, "disable_subtitle")) {
            p.Shared_libs = append(p.Shared_libs, "libzvbi")
        }

        ctx.AppendProperties(p)
    })
    return module
}

func am_mw_defaults() (android.Module) {
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

        if (strings.Contains(adtv_config, "disable_local_db")) {
            fmt.Println("local_db disabled in am_mw")
            p.Cflags = append(p.Cflags, "-DDISABLE_LOCAL_DB")
            db_srcs := []string{"am_db/am_db.c"}
            p.Exclude_srcs = append(p.Exclude_srcs, db_srcs...)
        } else {
            //fmt.Println("db defaults added")
        }

        if (strings.Contains(adtv_config, "disable_subtitle")) {
            fmt.Println("subtitle disabled in am_mw")
            p.Cflags = append(p.Cflags, "-DDISABLE_SUBTITLE")
            subtitle_srcs := []string{
                "am_sub2/am_sub.c",
                "am_sub2/dvb_sub.c",
                "am_tt2/am_tt.c",
                "am_cc/am_cc.c",
                "am_cc/cc_json.c",
                "am_closecaption/am_cc.c",
                "am_closecaption/am_cc_decoder.c",
                "am_closecaption/am_xds.c",
                "am_closecaption/am_cc_slice.c",
                "am_closecaption/am_vbi/linux_vbi/linux_vbi.c",
                "am_closecaption/am_vbi/am_vbi_api.c",
            }
            p.Exclude_srcs = append(p.Exclude_srcs, subtitle_srcs...)
        } else {
            //fmt.Println("sub defaults added")
        }

        if (strings.Contains(adtv_config, "disable_misc")) {
            fmt.Println("misc disabled in am_mw")
            p.Cflags = append(p.Cflags, "-DDISABLE_MISC")
            misc_srcs := []string{
                    "am_caman/am_caman.c",
                    "am_caman/ca_dummy.c",
                    "am_upd/am_upd.c",
                }
            p.Exclude_srcs = append(p.Exclude_srcs, misc_srcs...)
        } else {
            //fmt.Println("misc defaults added")
        }


        ctx.AppendProperties(p)
    })
    return module
}

