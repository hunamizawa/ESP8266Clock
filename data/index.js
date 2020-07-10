/*global $, tz_cities*/

const debug = document.location.protocol === 'file:';

const setting_mock = {
  "pane": "DATE_TIME",
  "override_pane": "NORMAL",
  "brightness": {
    "manual_value": 3,
    "thresholds": [1024, 360, 270, 200, 160, 120],
    "hysteresis": 10
  },
  "tzarea": "Asia",
  "tzcity": "Tokyo",
  "ntp": ["ntp.nict.jp"],
  "elev": 0,
  "use_ambient": false,
  "ambient_channelid": 100,
  "ambient_writekey": "123456789012345678",
  "use_custom_server": false,
  "custom_server_addr": "http://example.com/",
  "custom_server_writekey": "123456789012345678",
};

const brightness_mock = {
  "brightness": 4,
  "adc": 210,
};

function changeTzCities(area) {
  $('#tzcity > option').remove();
  if (area && tz_cities[area]) {
    for (const city of tz_cities[area]) {
      $('#tzcity').append(`<option value="${city}">${city}</option>`);
    }
  }
}

function toggleVisibillityWithCheckbox(checkbox, slave, reverse = false) {
  if (checkbox.prop('checked') != reverse)
    slave.show();
  else
    slave.hide();
}

function assignHandlerWithCheckbox(checkbox, slave, reverse = false) {
  checkbox.on('change', function () {
    if (checkbox.prop('checked') != reverse)
      slave.show(500);
    else
      slave.hide(500);
  });
}

function prepareComponents() {
  $('.require-reboot').on('submit', function () {
    return confirm('設定を反映させるため、時計を再起動します。');
  });

  for (const area in tz_cities) {
    $('#tzarea').append(`<option value="${area}">${area}</option>`);
  }
  $('#tzarea').on('change', function () {
    const area = $('#tzarea > option:selected').val();
    changeTzCities(area);
  });

  $('#panes').on('click', function (e) {
    const pane = $(e.target).val();
    if (pane)
      postSettings({ pane });
  });

  $('#override_panes').on('click', function (e) {
    const override_pane = $(e.target).val();
    if (override_pane)
      postSettings({ override_pane });
  });

  $('#auto-brightness').on('change', function () {
    const auto_brightness = $('#auto-brightness').prop('checked');
    postSettings({ auto_brightness });
  });

  $('#brightness-range').on('input', function () {
    const val = $('#brightness-range').val();
    $('#brightness').text(val < 0 ? 'オフ' : val);
  });

  $('#brightness-range').on('change', function () {
    if (!$('#brightness-range').prop('disabled')) {
      const manual_brightness = $('#brightness-range').val();
      postSettings({ manual_brightness });
    }
  });

  for (let i = 0; i <= 5; i++) {
    $(`#br-threshold-${i}-range`).on('input', function () {
      const val = $(`#br-threshold-${i}-range`).val();
      $(`#br-threshold-${i}`).text(val);
    });
  }

  assignHandlerWithCheckbox($('#use-ambient'), $('#group-ambient-channelid'));
  assignHandlerWithCheckbox($('#use-ambient'), $('#group-ambient-writekey'));
  assignHandlerWithCheckbox($('#use-custom-server'), $('#group-custom-server-addr'));
  assignHandlerWithCheckbox($('#use-custom-server'), $('#group-custom-server-writekey'));
}

function toggle_btn_group(selector, value) {
  $(`${selector} input[value=${value}]`)
    .prop('checked', true)
    .parent().addClass('active');
  $(`${selector} input[value!=${value}]`)
    .parent().removeClass('active');
}

function setSettingValues(setting) {

  $(`#tzarea > option[value=${setting.tzarea}]`).prop('selected', true);
  changeTzCities(setting.tzarea);
  $(`#tzcity > option[value=${setting.tzcity}]`).prop('selected', true);

  setDisplaySetting(setting, true);

  for (let i = 0; i <= 5; i++) {
    const val = setting.brightness.thresholds[i];
    $(`#br-threshold-${i}`).text(val);
    $(`#br-threshold-${i}-range`).val(val);
  }

  $('#ntp1').val(setting.ntp[0]);
  if (setting.ntp.length > 1)
    $('#ntp2').val(setting.ntp[1]);
  if (setting.ntp.length > 2)
    $('#ntp3').val(setting.ntp[2]);

  $('#elev').val(setting.elev);

  $('#use-ambient').prop('checked', setting.use_ambient);
  $('#ambient-channelid').val(setting.ambient_channelid);
  $('#ambient-writekey').val(setting.ambient_writekey);

  $('#use-custom-server').prop('checked', setting.use_custom_server);
  $('#custom-server-addr').val(setting.custom_server_addr);
  $('#custom-server-writekey').val(setting.custom_server_writekey);

  toggleVisibillityWithCheckbox($('#use-ambient'), $('#group-ambient-channelid'));
  toggleVisibillityWithCheckbox($('#use-ambient'), $('#group-ambient-writekey'));
  toggleVisibillityWithCheckbox($('#use-custom-server'), $('#group-custom-server-addr'));
  toggleVisibillityWithCheckbox($('#use-custom-server'), $('#group-custom-server-writekey'));

}

function beforeLoad() {

  $('.disable-until-load').prop('disabled', true);
  $('.disable-on-load').prop('disabled', true);
}

function afterLoad() {

  $('.disable-until-load').prop('disabled', false);
}

function loadAllSettings(cb) {

  beforeLoad();

  if (debug) {

    setSettingValues(setting_mock);
    setBrightnessInfo(brightness_mock);

    afterLoad();

    if (cb)
      cb();
  } else {

    $.when(
      $.getJSON('/setting', setSettingValues),
      $.getJSON('/brightness', setBrightnessInfo)
    ).done(() => {
      afterLoad();
    }).always(() => {
      if (cb)
        cb();
    });
  }
}

function postSettings(data, cb) {

  beforeLoad();

  if (debug) {

    console.log(data);
    setSettingValues(setting_mock);
    afterLoad();
    if (cb)
      cb();
    
  } else {

    if (reloadTimer == null) {
      setTimeout(() => postSettings(data, cb), 100);
      return;
    }

    clearTimeout(reloadTimer);
    reloadTimer = null;

    $.ajax({
      method: 'POST',
      dataType: 'json',
      url: '/setting',
      data: data,
      success: setSettingValues
    }).always(() => {
      afterLoad();
      reloadTimer = setTimeout(reloadDisplayInfo, 1000);
      if (cb)
        cb();
    });
  }
}

function setDisplaySetting(setting) {

  toggle_btn_group('#panes', setting.pane);
  toggle_btn_group('#override_panes', setting.override_pane);

  const manual_value = setting.brightness.manual_value;
  const auto_brightness = manual_value == -1;
  $('#auto-brightness').prop('checked', auto_brightness);
  $('#brightness-range').prop('disabled', auto_brightness);
  if (!auto_brightness) {
    $('#brightness').text(manual_value < 0 ? 'オフ' : manual_value);
    $('#brightness-range').val(manual_value);
  }
}

function setBrightnessInfo(data) {

  if ($('#brightness-range').prop('disabled')) {
    $('#brightness').text(data.brightness < 0 ? 'オフ' : data.brightness);
    $('#brightness-range').val(data.brightness);
  }
  $('#adc').text(data.adc);
  $('#adc-meter').val(data.adc);
}

let reloadTimer = null;

function reloadDisplayInfo() {

  reloadTimer = null;

  if (debug) {

    setDisplaySetting(setting_mock);
    setBrightnessInfo(brightness_mock);
    reloadTimer = setTimeout(reloadDisplayInfo, 1000);
  } else {

    $.when(
      $.getJSON('/setting', setDisplaySetting),
      $.getJSON('/brightness', setBrightnessInfo)
    ).always(() => reloadTimer = setTimeout(reloadDisplayInfo, 1000));
  }
}

$(function () {
  prepareComponents();
  loadAllSettings(() => {
    reloadTimer = setTimeout(reloadDisplayInfo, 1000);
  });
});
