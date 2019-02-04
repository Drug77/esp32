<template>
  <div id="app">
    <led-control />
  </div>
</template>

<script>
import Vue from 'vue'
import Hammer from 'hammerjs'
import VueCircleSlider from 'vue-circle-slider'

import LedControl from './components/LedControl'

Vue.use(VueCircleSlider)

Vue.directive('double-tap', {
    bind: function(el, binding) {
        if (typeof binding.value === 'function') {
            const mc = new Hammer(el);
            mc.on('doubletap', binding.value);
        }
    }
});

export default {
  name: 'app',
  components: {
      LedControl
  }
}
</script>

<!-- Add "scoped" attribute to limit CSS to this component only -->
<style lang="scss">
  @import '@/scss/app.scss';
</style>