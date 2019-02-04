<template>
    <div class="color-picker">
        <div class="color-chip" v-bind:style="{'background': color}">
            <div class="color-chip__inner">
                <h1 class="color-chip__title">HSL</h1>
                <h3 class="color-chip__subtitle">{{ colorString }}</h3>
            </div>
        </div>
        <div class="color-picker__inner">
            <div class="color-picker__control" v-bind:style="gradientH">
                <input type="range" min="0" max="360" v-model="h"/>
            </div>
            <div class="color-picker__control" v-bind:style="gradientS">
                <input type="range" min="0" max="100" v-model="s"/>
            </div>
            <div class="color-picker__control" v-bind:style="gradientL">
                <input type="range" min="0" max="100" v-model="l"/>
            </div>
        </div>
    </div>
</template>

<script>
    export default {
        name: 'color-picker',
        props: ['change', 'initial'],
        data() {
            return {
                h: 265,
                s: 80,
                l: 99,
            }
        },
        computed: {
            color() {
                let hsl = hsb2hsl(parseFloat(this.h) / 360, parseFloat(this.s) / 100, parseFloat(this.l) / 100);
                let c = hsl.h + ', ' + hsl.s + '%, ' + hsl.l + '%';

                let rgb = hslToRgb(parseFloat(hsl.h) / 360, parseFloat(hsl.s) / 100, parseFloat(hsl.l) / 100);
                this.$emit('input', rgb);
                this.$emit('change', rgb);

                let s = 'hsl(' + c + ')';
                return s;
            },
            colorString() {
                let c = this.h + ', ' + this.s + '%, ' + this.l + '%';

                return c;
            },
            gradientH() {
                let stops = [];

                for (let i = 0; i < 7; i++) {
                    let h = i * 60;

                    let hsl = hsb2hsl(parseFloat(h / 360), parseFloat(this.s) / 100, parseFloat(this.l / 100));

                    let c = hsl.h + ', ' + hsl.s + '%, ' + hsl.l + '%';
                    stops.push('hsl(' + c + ')');
                }

                return {
                    backgroundImage: 'linear-gradient(to right, ' + stops.join(', ') + ')'
                }
            },
            gradientS() {
                let stops = [];
                let c;
                let hsl = hsb2hsl(parseFloat(this.h / 360), 0, parseFloat(this.l / 100))
                c = hsl.h + ', ' + hsl.s + '%, ' + hsl.l + '%'
                stops.push('hsl(' + c + ')')

                hsl = hsb2hsl(parseFloat(this.h / 360), 1, parseFloat(this.l / 100))
                c = hsl.h + ', ' + hsl.s + '%, ' + hsl.l + '%'
                stops.push('hsl(' + c + ')')

                return {
                    backgroundImage: 'linear-gradient(to right, ' + stops.join(', ') + ')'
                }
            },
            gradientL() {
                let stops = [];
                let c;

                let hsl = hsb2hsl(parseFloat(this.h / 360), 0, 0)
                c = hsl.h + ', ' + hsl.s + '%, ' + hsl.l + '%'
                stops.push('hsl(' + c + ')')

                hsl = hsb2hsl(parseFloat(this.h / 360), parseFloat(this.s / 100), 1)

                c = hsl.h + ', ' + hsl.s + '%, ' + hsl.l + '%'
                stops.push('hsl(' + c + ')')

                return {
                    backgroundImage: 'linear-gradient(to right, ' + stops.join(', ') + ')'
                }
            }
        },
        mounted() {
            this.h = parseInt(Math.random() * 360)
        }
    }

    function hsb2hsl(h, s, b) {
        let hsl = {
            h: h
        };
        hsl.l = (2 - s) * b;
        hsl.s = s * b;

        if (hsl.l <= 1 && hsl.l > 0) {
            hsl.s /= hsl.l;
        } else {
            hsl.s /= 2 - hsl.l;
        }

        hsl.l /= 2;

        if (hsl.s > 1) {
            hsl.s = 1;
        }

        if (!hsl.s > 0) hsl.s = 0


        hsl.h *= 360;
        hsl.s *= 100;
        hsl.l *= 100;

        return hsl;
    }

    /**
     * Converts an HSL color value to RGB. Conversion formula
     * adapted from http://en.wikipedia.org/wiki/HSL_color_space.
     * Assumes h, s, and l are contained in the set [0, 1] and
     * returns r, g, and b in the set [0, 255].
     *
     * @param   {number}  h       The hue
     * @param   {number}  s       The saturation
     * @param   {number}  l       The lightness
     * @return  {Array}           The RGB representation
     */
    function hslToRgb(h, s, l) {
        let r, g, b;

        if (s == 0) {
            r = g = b = l; // achromatic
        } else {
            let hue2rgb = function hue2rgb(p, q, t) {
                if (t < 0) t += 1;
                if (t > 1) t -= 1;
                if (t < 1 / 6) return p + (q - p) * 6 * t;
                if (t < 1 / 2) return q;
                if (t < 2 / 3) return p + (q - p) * (2 / 3 - t) * 6;
                return p;
            }

            let q = l < 0.5 ? l * (1 + s) : l + s - l * s;
            let p = 2 * l - q;
            r = hue2rgb(p, q, h + 1 / 3);
            g = hue2rgb(p, q, h);
            b = hue2rgb(p, q, h - 1 / 3);
        }

        return [Math.round(r * 255), Math.round(g * 255), Math.round(b * 255)];
    }
</script>

<!-- Add "scoped" attribute to limit CSS to this component only -->
<style scoped lang="scss">
    .color-picker {
        width: 100%;
    }

    .color-picker__inner {
        padding: 1.5rem 1rem;
    }

    .color-chip {
        height: 260px;
        display: flex;
        justify-content: center;
        align-items: center;
        color: white;
        border-radius: 4px 4px 0 0;
    }

    .color-chip__inner {
        text-align: center;
    }

    .color-chip__subtitle {
        margin-top: 0.5rem;
        opacity: 0.7;
        font-weight: normal;
        font-size: 2rem;
        text-shadow: 0px 1px 2px rgba(0, 0, 0, 0.5);
    }

    .color-chip__title {
        color: white;
        margin: 0;
        font-size: 3rem;
        letter-spacing: 4px;
        text-shadow: 0px 1px 2px rgba(0, 0, 0, 0.5);
    }

    .color-picker__control {
        width: 100%;
        height: 22px;
        border-radius: 12px;
        border: 1px solid $color-darker-blue;
    }

    .color-picker__control + .color-picker__control {
        margin-top: 1rem;
    }

    .color-picker__control input {
        width: 100%;
        margin: 0;
    }

    .color-picker__control input[type=range] {
        -webkit-appearance: none;
        width: 100%;
        background: transparent;
    }

    .color-picker__control input[type=range]:focus {
        outline: none;
    }

    .color-picker__control input[type=range]::-ms-track {
        width: 100%;
        cursor: pointer;
        background: transparent;
        border-color: transparent;
        color: transparent;
    }

    .color-picker__control input[type=range]::-webkit-slider-thumb {
        -webkit-appearance: none;
        border: 1px solid #ddd;
        height: 32px;
        width: 32px;
        border-radius: 50%;
        background: white;
        cursor: pointer;
        box-shadow: 0px 1px 2px rgba(0, 0, 0, 0.12);
        transform: translateY(-11px);
    }
</style>
