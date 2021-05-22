$(document).ready(function () {
    // initial disabling
    $(".form-floating > input").each(function (i, el) {
      if ($("#profile").children("option:selected").val() !== "Custom..." && !($(el).attr("id").endsWith("_t_ms"))) {
        $(el).prop("readonly", true);
      } else {
        $(el).prop("readonly", false);
      }
    });

    // Initial fetch
    $.get(
        "/load",
        { profileload: $("#profile").children("option:selected").val() },
        function (data) {
          Object.keys(data).forEach(param => {
              $(".form-floating > #" + param).val(data[param])
          });
        }
      );


    // fetch profile on profile change
    $("#profile").change(function () {
      if ($("#profile").children("option:selected").val() !== "Custom...") {
        $.get(
          "/load",
          { profileload: $("#profile").children("option:selected").val() },
          function (data) {
            Object.keys(data).forEach(param => {
                $(".form-floating > #" + param).val(data[param])
            });
          }
        );
      }

      // subsequent disabling
      $(".form-floating > input").each(function (i, el) {
        if ($("#profile").children("option:selected").val() !== "Custom..." && !($(el).attr("id").endsWith("_t_ms"))) {
          $(el).prop("readonly", true);
        } else {
          $(el).prop("readonly", false);
        }
      });
    });
  });