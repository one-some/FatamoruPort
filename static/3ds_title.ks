; Bottom
[claire_ctrswapscreen]

[image layer="base" storage="massageback" page="fore"]

[position layer="message1" page="fore" marginl="0"]
[current layer="message1"]

[locate x="10" y="10"]
[button native graphic="button_start" target="*scenario"]

[locate x="10" y="115"]
[button native graphic="button_inspect" target="*scenario"]

[locate x="10" y="188"]
[button native graphic="button_config" target="*scenario"]

[locate x="165" y="188"]
[button native graphic="button_extras" target="*scenario"]

; Somethings bugged out with the [position] page I think, so we have to do this switcheroo
[backlay]
[current layer="base"]
[image layer="base" storage="blacksozai" page="fore"]
[trans method="crossfade" time="3000" stay="nostay" children="true"]

; Top
[claire_ctrswapscreen]

; Okay lets just extend the language a bit
[image layer=base storage="オープニング" page="back"]
[trans method="crossfade" time="3000" stay="nostay" children="false"]
[wt]
[playbgm storage="女中のテーマピアノソロ"]

[image storage="オープニング" layer="base" page="fore"]
[backlay]

; TODO transition botttom in like top

[s]

*scenario
[image layer=base storage="blacksozai" page=back]
[trans method="crossfade" time="3000" stay="nostay" children="false"]

; Bottom
[claire_ctrswapscreen]

[image layer=base storage="blacksozai" page=back]
[trans method="crossfade" time="3000" stay="nostay" children="false"]

[wt]
[fadeoutbgm time="5000"]

; Top
[claire_ctrswapscreen]

[jump storage="scenario.ks" target="*start"]
