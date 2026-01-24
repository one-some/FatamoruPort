; Bottom
[claire_ctrswapscreen]

;[image layer="base" storage="massageback" page="back"]
[position layer="message1" page="back" marginl="0"]
[current layer="message1" page="back"]

[locate x="20" y="25"]
Hello bro
[button native graphic="button_start" target="*scenario"]

[locate x="20" y="287"]
[button native graphic="button_inspect" target="*scenario"]

[locate x="20" y="470"]
[button native graphic="button_config" target="*scenario"]

[locate x="330" y="470"]
[button native graphic="button_extras" target="*scenario"]

[trans method="crossfade" time="3000" stay="nostay" children="false"]

; Top
[claire_ctrswapscreen]

; Okay lets just extend the language a bit
[image layer=base storage="オープニング" page="back"]
[trans method="crossfade" time="3000" stay="nostay" children="false"]
[wt]
[playbgm storage="女中のテーマピアノソロ"]

[image storage="オープニング" layer="base" page="fore"]
[backlay]
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
