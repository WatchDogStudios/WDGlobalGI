{
    "Source" : "WDGlobalGIDebugView.azsl",

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
