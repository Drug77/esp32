<template>
    <div class="led-control" ref="parent" v-double-tap="modeNone">
        <button class="led-control__item" @click="toggle('color-wipe', $event)">Color Wipe</button>
        <button class="led-control__item" @click="toggle('theatre-chase', $event)">Theatre Chase</button>
        <button class="led-control__item" @click="toggle('rainbow', $event)">Rainbow</button>
        <button class="led-control__item" @click="toggle('rainbow-cycle', $event)">Rainbow Cycle</button>
        <button class="led-control__item" @click="toggle('candle', $event)">Candle</button>
        <button class="led-control__item" @click="toggle('tungsten-40w', $event)">Tungsten 40w</button>
        <button class="led-control__item" @click="toggle('tungsten-100w', $event)">Tungsten 100w</button>
        <button class="led-control__item" @click="toggle('halogen', $event)">Halogen</button>
        <button class="led-control__item" @click="toggle('carbon-arc', $event)">Carbon Arc</button>
        <button class="led-control__item" @click="toggle('high-noon-sun', $event)">High Noon Sun</button>
        <button class="led-control__item" @click="toggle('pure-white', $event)">Pure White</button>
        <button class="led-control__item" @click="toggle('overcast-sky', $event)">Overcast Sky</button>
        <button class="led-control__item" @click="toggle('clear-blue-sky', $event)">Clear Blue Sky</button>
        <button class="led-control__item" @click="toggle('color', $event)">Color</button>
        <button class="led-control__item" @click="toggle('none', $event)">None</button>
        <div class="led-control__setting" v-show="settingsColor">
            <color-picker v-model="rgb"></color-picker>
        </div>
        <div class="led-control__setting mt-2" v-show="settingsBrightness">
            <span class="led-control__label-1">Brightness</span>
            <circle-slider
                    :min="1"
                    :max="100"
                    v-model="data.brightness"
                    progress-color="#94c325"
                    knob-color="#94c325"
                    :side="300"
            ></circle-slider>
            <span class="led-control__label-2">{{ data.brightness }}%</span>
        </div>
        <div class="led-control__setting mb-2" v-show="settingsSpeed">
            <span class="led-control__label-1">Speed</span>
            <circle-slider
                    :min="10"
                    :max="500"
                    v-model="data.speed"
                    progress-color="#94c325"
                    knob-color="#94c325"
                    :side="300"
            ></circle-slider>
            <span class="led-control__label-2">{{ data.speed }}</span>
        </div>
    </div>
</template>

<script>
    import axios from 'axios';
    import throttle from 'lodash/throttle';

    import ColorPicker from './ColorPicker';

    export default {
        name: 'led-control',
        components: {
            ColorPicker
        },
        data() {
            return {
                rgb: null,
                settingsColor: false,
                settingsBrightness: false,
                settingsSpeed: false,
                data: {
                    mode: 'none',
                    brightness: 100,
                    speed: 50,
                    red: 0,
                    green: 0,
                    blue: 0,
                },
            }
        },
        watch: {
            'data.brightness': function () { this.request() },
            'data.speed': function () { this.request() },
            rgb(v) {
                this.data.red = v[0];
                this.data.green = v[1];
                this.data.blue = v[2];

                this.request();
            }
        },
        methods: {
            modeNone() {
                this.data.mode = 'none';
                this.data.brightness = 20;
                this.request();
            },
            toggle(mode, e) {
                let children = this.$refs.parent.children;
                let isOpen = false;

                if (e.target.classList.contains('led-control__item--active')) {
                    e.target.classList.remove('led-control__item--active');

                    for (let c of children) {
                        if (c.classList.contains('led-control__item') && c != e.target) {
                            c.classList.remove('led-control__item--shrink');
                        }
                    }

                    isOpen = false;
                } else {
                    e.target.classList.add('led-control__item--active');

                    for (let c of children) {
                        if (c.classList.contains('led-control__item') && c != e.target) {
                            c.classList.add('led-control__item--shrink');
                        }
                    }

                    isOpen = true;
                }

                this.mode(mode, isOpen);
                this.request();
            },
            mode(mode, isOpen) {
                this.data.mode = mode;

                if (isOpen) {
                    if ([
                        'candle', 'tungsten-40w', 'tungsten-100w', 'halogen',
                        'carbon-arc', 'high-noon-sun', 'pure-white', 'overcast-sky', 'clear-blue-sky'
                    ].indexOf(mode) !== -1) {
                        this.settingsColor = false;
                        this.settingsBrightness = true;
                        this.settingsSpeed = false;
                    }
                    else if (['rainbow', 'rainbow-cycle'].indexOf(mode) !== -1) {
                        this.settingsColor = false;
                        this.settingsBrightness = true;
                        this.settingsSpeed = true;
                    }
                    else if (['color-wipe', 'theatre-chase'].indexOf(mode) !== -1) {
                        this.settingsColor = true;
                        this.settingsBrightness = false;
                        this.settingsSpeed = true;

                        this.data.brightness = 100;
                    }
                    else if (['color'].indexOf(mode) !== -1) {
                        this.settingsColor = true;
                        this.settingsBrightness = false;
                        this.settingsSpeed = false;

                        this.data.brightness = 100;
                    }
                    else if (['none'].indexOf(mode) !== -1) {
                        this.settingsColor = false;
                        this.settingsBrightness = false;
                        this.settingsSpeed = false;
                    }
                } else {
                    this.settingsColor = false;
                    this.settingsBrightness = false;
                    this.settingsSpeed = false;
                }
            },
            request: throttle(function () {
                axios({
                    method: 'post',
                    url: location.protocol + '//' + location.host + '/api/command',
                    headers: {
                        'Content-Type': 'application/json'
                    },
                    data: this.data
                })
                    .then(response => {
                        if (response.status === 200) {
                            // eslint-disable-next-line
                            console.log(response.data);
                        }
                    })
                    .catch(error => {
                        // eslint-disable-next-line
                        console.log(error);
                    })
                    .finally(() => {

                    });
            }, 800)
        }
    }
</script>

<!-- Add "scoped" attribute to limit CSS to this component only -->
<style scoped lang="scss">
    .led-control {
        align-items: center;
        display: flex;
        flex-direction: column;
        font-size: 2.6rem;

        &__item {
            font-size: inherit;
            width: 100%;
            text-align: center;
            height: calc(100vh / 4);
            overflow: hidden;
            transition: height .3s ease-out;
        }

        &__item:not(:first-child) {
            box-shadow: inset 0px 10px 30px -8px rgba(0, 0, 0, 0.2);
        }

        &__item:nth-child(4n+1) {
            background-color: $color-purple;
        }

        &__item:nth-child(4n+2) {
            background-color: $color-red;
        }

        &__item:nth-child(4n+3) {
            background-color: $color-green;
        }

        &__item:nth-child(4n+4) {
            background-color: $color-blue;
        }

        &__item--active {
            height: auto;
            padding: .5rem;
            transition: height .3s ease-out;
        }

        &__item--shrink {
            height: 0;
            transition: height .3s ease-out;
        }

        &__setting {
            align-items: center;
            display: flex;
            flex-direction: column;
            position: relative;
            width: 100%;
        }

        &__label-1 {
            font-size: 2rem;
            margin-bottom: 1rem;
        }

        &__label-2 {
            position: absolute;
            bottom: 151px;
            transform: translateY(50%);
        }
    }
</style>
