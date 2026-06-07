{
    "Source" : "WDGlobalGIVolumetricComposite.azsl",

    "RasterState" :
    {
        "CullMode" : "Back"
    },

    "DepthStencilState" :
    {
        "Depth" :
        {
            "Enable" : false
        },
        "Stencil" :
        {
            "Enable" : false
        }
    },

    "GlobalTargetBlendState" : {
        "Enable" : true,
        "BlendSource" : "One",
        "BlendDest" : "One",
        "BlendOp" : "Add",
        "BlendAlphaSource" : "Zero",
        "BlendAlphaDest" : "One",
        "BlendAlphaOp" : "Add"
    },

    "DrawList" : "forward",

    "ProgramSettings" :
    {
        "EntryPoints" :
        [
            { "name" : "MainVS", "type" : "Vertex" },
            { "name" : "MainPS", "type" : "Fragment" }
        ]
    },

    "Supervariants" :
    [
        {
            "Name" : "NoMSAA",
            "AddBuildArguments" : {
                "azslc" : ["--no-ms"]
            }
        }
    ]
}
